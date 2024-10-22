#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "d2d_sync.h"
#include "systimer.h"
#include "delay.h"
#include "serial_reg.h"
#include "dw_apb_timers.h"
#include "gicv3.h"

#define INT_TO_FIXPT(x)         ((x) << 16)
#define FIXPT_TO_INT(x)         ((x) >> 16)

#define do_div(n, base) ({						\
	unsigned int __base = (base);					\
	unsigned int __rem;						\
	__rem = ((unsigned long long)(n)) % __base;			\
	(n) = ((unsigned long long)(n)) / __base;			\
	__rem;								\
})

static uint64_t calculate_KBs(uint64_t time_us, size_t len)
{
	uint64_t per_sec = 1000000;

	if (time_us <= 0)
		return 0;

	/* drop precision until time_us is 32-bits */
	while (time_us > (~0U)) {
		time_us >>= 1;
		per_sec <<= 1;
	}

	per_sec *= (len >> 10);
	per_sec = INT_TO_FIXPT(per_sec);
	do_div(per_sec, time_us);

	return FIXPT_TO_INT(per_sec);
}

void dump_mem(const void *mem, size_t length) {
    const unsigned char *byte = (const unsigned char *)mem;
    for (size_t i = 0; i < length; i++) {
        if (i % 16 == 0) {
            printf("[0x%06lx] ", i);
        }
        printf("%02x ", byte[i]);
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
    if (length % 16 != 0) {
        printf("\n");
    }
}

#define TEST_DATA_BUFL  (0x40000000)
#define TEST_DATA_BUFR  (0x40100000)
#define TEST_NPU_ADDR   (0x0800020000)
#define TEST_DATA_LEN   (0x200)
#define TEST_ARRL(i)  *((unsigned char *) ((uintptr_t) (TEST_DATA_BUFL + (i))))
#define TEST_ARRR(i)  *((unsigned char *) ((uintptr_t) (TEST_DATA_BUFR + (i))))

static int d2d_basic_func_test(void)
{
    int ret = 0, i;

    printf("=== %s\n", __func__);

    if (CONFIG_RHEA_D2D_SELF_ID == RHEA_DIE0_ID) {
        ret = rhea_d2d_select_tile(RHEA_DIE1_IDX, 0x4, 0);
        if (ret)
            goto err;
        
        for (i = 0; i < TEST_DATA_LEN; i++)
            TEST_ARRL(i) = i & 0xff;
        printf("=== Write 0x%x bytes data from 0x%x to remote 0x%x\n",
            TEST_DATA_LEN, TEST_DATA_BUFL, TEST_DATA_BUFR);
        ret = rhea_d2d_write_data((void *) TEST_DATA_BUFL, 
            TEST_DATA_BUFR, TEST_DATA_LEN);
        if (ret)
            goto err;

#if defined(CONFIG_RHEA_D2D_LOOKBACK)
        for (i = 0; i < TEST_DATA_LEN; i++) {
            if (TEST_ARRR(i) != (i & 0xff)) {
                printf("=== Remote verification failed, got %d at index %d, expected %d\n",
                        TEST_ARRR(i), i, i & 0xff);
                ret = -EIO;
                goto err;
            }
        }
#endif

        memset((void *) TEST_DATA_BUFL, 0, TEST_DATA_LEN);
        printf("=== Read 0x%x bytes data from remote 0x%x to 0x%x\n",
            TEST_DATA_LEN, TEST_DATA_BUFR, TEST_DATA_BUFL);
        ret = rhea_d2d_read_data((void *) TEST_DATA_BUFL, 
            TEST_DATA_BUFR, TEST_DATA_LEN);
        if (ret)
            goto err;

        for (i = 0; i < TEST_DATA_LEN; i++) {
            if (TEST_ARRL(i) != (i & 0xff)) {
                printf("=== Local verification failed, got %d at index %d, expected %d\n",
                        TEST_ARRL(i), i, i & 0xff);
                ret = -EIO;
                goto err;
            }
        }
        printf("=== %s verification successful\n\n", __func__);
err:
    rhea_d2d_release_tile();
    } else {
        mdelay(1000);
        printf("=== Slave die, test skip\n");
    }
    return ret;
}

static int d2d_sync_data_test(void)
{
    int ret, i, timeout = 50;
    struct d2d_sync_put_cmd put_cmd;
    uint32_t size;

    printf("=== %s\n", __func__);

#if !defined(CONFIG_RHEA_D2D_LOOKBACK)
    if (CONFIG_RHEA_D2D_SELF_ID == RHEA_DIE0_ID) {
#endif
        for (i = 0; i < TEST_DATA_LEN; i++)
            TEST_ARRL(i) = i & 0xff;
        put_cmd.die_idx = RHEA_DIE1_IDX;
        put_cmd.cmd_id = D2D_SYNC_DATA;
        put_cmd.data_addr = (void *) ((uintptr_t) TEST_DATA_BUFL);
        put_cmd.data_size = TEST_DATA_LEN;
        printf("=== Write 0x%x bytes data from 0x%x to remote\n",
            TEST_DATA_LEN, TEST_DATA_BUFL);
        ret = d2d_sync_remote(&put_cmd);
        if (ret)
            return ret;
#if !defined(CONFIG_RHEA_D2D_LOOKBACK)
    } else {
#endif
        printf("=== Obtain command\n");
        while (timeout--) {
            ret = d2d_sync_obtain_cmd();
            if (ret < 0) return ret;
            else if (ret == 0) continue;

            ret = d2d_sync_query_cmd(D2D_SYNC_DATA, &size);
            if (ret < 0) return ret;
            else if (ret == D2D_SYNC_DATA) break;

            mdelay(10);
        }
        if (!timeout) return -ETIMEDOUT;

        memset((void *) TEST_DATA_BUFL, 0, TEST_DATA_LEN);
        printf("=== Read 0x%x bytes data from remote to 0x%x\n",
            size, TEST_DATA_BUFL);
        ret = d2d_sync_get_data(D2D_SYNC_DATA, 
                            (void *) TEST_DATA_BUFL);
        if (ret != D2D_SYNC_DATA)
            return ret;

        for (i = 0; i < TEST_DATA_LEN; i++) {
            if (TEST_ARRL(i) != (i & 0xff)) {
                printf("=== Verification failed, got %d at index %d, expected %d\n",
                        TEST_ARRL(i), i, i & 0xff);
                return -EIO;
            }
        }
#if !defined(CONFIG_RHEA_D2D_LOOKBACK)
    }
#endif
    printf("=== %s verification successful\n\n", __func__);
    return 0;
}

static int d2d_sync_reg(struct d2d_sync_put_cmd *put_cmd)
{
    int ret, pos;

    ret = d2d_sync_remote(put_cmd);
    if (ret < 0)
        return ret;
    pos = ret;

#if defined(CONFIG_RHEA_D2D_LOOKBACK)
    if (d2d_sync_obtain_cmd() < 0)
        return -EIO;
#endif
#if !defined(CONFIG_RHEA_D2D_LOOKBACK)
    ret = rhea_d2d_select_tile(RHEA_DIE1_IDX, 0x4, 0);
    if (ret < 0)
        return ret;
#endif
    ret = d2d_sync_wait_reg(put_cmd, pos);
#if !defined(CONFIG_RHEA_D2D_LOOKBACK)
    rhea_d2d_release_tile();
#endif
    return ret;
}

static int d2d_sync_writel(uint32_t val, uint32_t addr)
{
    int ret;
    struct d2d_sync_put_cmd put_cmd;

    put_cmd.die_idx = 1;
    put_cmd.cmd_id = D2D_SYNC_WRITEL;
    put_cmd.reg_addr = addr;
    put_cmd.reg_val = val;
    return d2d_sync_reg(&put_cmd);
}

static int d2d_sync_readl(uint32_t *val, uint32_t addr)
{
    int ret;
    struct d2d_sync_put_cmd put_cmd;

    put_cmd.die_idx = 1;
    put_cmd.cmd_id = D2D_SYNC_READL;
    put_cmd.reg_addr = addr;
    put_cmd.reg_val = 0;
    ret = d2d_sync_reg(&put_cmd);
    *val = put_cmd.reg_val;
    return ret;
}

static int d2d_sync_writew(uint16_t val, uint32_t addr)
{
    int ret;
    struct d2d_sync_put_cmd put_cmd;

    put_cmd.die_idx = 1;
    put_cmd.cmd_id = D2D_SYNC_WRITEL;
    put_cmd.reg_addr = addr;
    put_cmd.reg_val = val;
    return d2d_sync_reg(&put_cmd);
}

static int d2d_sync_readw(uint16_t *val, uint32_t addr)
{
    int ret;
    struct d2d_sync_put_cmd put_cmd;

    put_cmd.die_idx = 1;
    put_cmd.cmd_id = D2D_SYNC_READL;
    put_cmd.reg_addr = addr;
    put_cmd.reg_val = 0;
    ret = d2d_sync_reg(&put_cmd);
    *val = (uint16_t) put_cmd.reg_val;
    return ret;
}

static int d2d_sync_writeb(uint8_t val, uint32_t addr)
{
    int ret;
    struct d2d_sync_put_cmd put_cmd;

    put_cmd.die_idx = 1;
    put_cmd.cmd_id = D2D_SYNC_WRITEL;
    put_cmd.reg_addr = addr;
    put_cmd.reg_val = val;
    return d2d_sync_reg(&put_cmd);
}

static int d2d_sync_readb(uint8_t *val, uint32_t addr)
{
    int ret;
    struct d2d_sync_put_cmd put_cmd;

    put_cmd.die_idx = 1;
    put_cmd.cmd_id = D2D_SYNC_READL;
    put_cmd.reg_addr = addr;
    put_cmd.reg_val = 0;
    ret = d2d_sync_reg(&put_cmd);
    *val = (uint8_t) put_cmd.reg_val;
    return ret;
}

static int d2d_sync_reg_test(uint64_t addr)
{
    int ret;
    struct d2d_sync_put_cmd put_cmd;
    enum d2d_sync_cmd_id cmd_id;
    static uint8_t i = 1;
    uint32_t mask = 0;
    uint32_t val = 0, read_back;
    uint32_t cmd_cnt = 0, timeout = 50;

    printf("=== %s with address 0x%010lx\n", __func__, addr);

#if !defined(CONFIG_RHEA_D2D_LOOKBACK)
    if (CONFIG_RHEA_D2D_SELF_ID == RHEA_DIE0_ID) {
#endif
        for (cmd_id = D2D_SYNC_WRITEL; 
                cmd_id < D2D_SYNC_CMD_MAX; cmd_id++, i++) {
            put_cmd.die_idx = 1;
            put_cmd.cmd_id = cmd_id;
            put_cmd.reg_addr = addr;
            if (cmd_id == D2D_SYNC_WRITEL) {
                val = (i << 24) | (i << 16) | (i << 8) | i;
                put_cmd.reg_val = val;
                mask = 0xFFFFFFFF;
            } else if (cmd_id == D2D_SYNC_WRITEW) {
                val = (i << 8) | i;
                put_cmd.reg_val = val;
                mask = 0xFFFF;
            } else if (cmd_id == D2D_SYNC_WRITEB) {
                val = i;
                put_cmd.reg_val = val;
                mask = 0xFF;
            } else {
                put_cmd.reg_val = 0;
                mask = 0x0;
            }
            printf("=== Command %d test (val: 0x%x)\n", cmd_id, val);
            ret = d2d_sync_reg(&put_cmd);
            if (ret)
                return ret;

            if (!mask) {
                if (put_cmd.reg_val != val) {
                    printf("=== Read command %d verification failed, got 0x%x, expected 0x%x\n",
                            put_cmd.cmd_id, put_cmd.reg_val, val);
                    return -EIO;
                }
            }
#if defined(CONFIG_RHEA_D2D_LOOKBACK)
            else {
                read_back = *((uint32_t *) addr) & mask;
                if (read_back != val) {
                    printf("=== Write command %d verification failed, got 0x%x, expected 0x%x\n",
                            put_cmd.cmd_id, read_back, val);
                    return -EIO;
                }
            }
#endif
        }
#if !defined(CONFIG_RHEA_D2D_LOOKBACK)
    } else {
        while (cmd_cnt < 6 && timeout--) {
            ret = d2d_sync_obtain_cmd();
            if (ret < 0) return ret;
            else if (ret != 0) {
                cmd_cnt++;
                printf("=== processed command (%d/6)\n", cmd_cnt);
            }

            mdelay(10);
        }
        if (!timeout) return -ETIMEDOUT;
    }
#endif
    printf("=== %s verification successful\n\n", __func__);
    return 0;
}

#define IOMUX_UART2_TX      (0x1200089c)
#define IOMUX_UART2_RX      (0x120008a0)
#define UART2_REG(x)        (0x10360000 + (x << 2))
#define UART_DLL_VAL        (0x0d) /* clock: 20000000, baud rate: 115200 */
#define UART_DLM_VAL        (0x00)
#define UART_DLF_VAL        (0x09)
#define DW_UART_DLF	        (0x30) /* Divisor Latch Fraction Register */
#define UART_THRE_TIMEOUT   (100)
static int d2d_sync_uart_test(void)
{
    static uint8_t i = 0;
    uint32_t val, timeout_ms = 0;

    printf("=== %s\n", __func__);

#if !defined(CONFIG_RHEA_D2D_LOOKBACK)
    if (CONFIG_RHEA_D2D_SELF_ID == RHEA_DIE0_ID) {
#endif
        /* IOMUX config */
        d2d_sync_readl(&val, IOMUX_UART2_RX);
        val = (val & ~0x70) | 0x20;
        d2d_sync_writel(val, IOMUX_UART2_RX);
        d2d_sync_readl(&val, IOMUX_UART2_TX);
        val = (val & ~0x70) | 0x20;
        d2d_sync_writel(val, IOMUX_UART2_TX);

        /* UART config */
        d2d_sync_readl(&val, UART2_REG(UART_LCR));
        val |= UART_LCR_DLAB;
        d2d_sync_writel(val, UART2_REG(UART_LCR));
        d2d_sync_writel(UART_DLL_VAL, UART2_REG(UART_DLL));
        d2d_sync_writel(UART_DLM_VAL, UART2_REG(UART_DLM));
        d2d_sync_writel(UART_DLF_VAL, UART2_REG(DW_UART_DLF));
        d2d_sync_readl(&val, UART2_REG(UART_LCR));
        val = (val & ~UART_LCR_DLAB) | UART_LCR_WLEN8;  // 115200 8N1
        d2d_sync_writel(val, UART2_REG(UART_LCR));
        val = UART_FCR_R_TRIG_11 | UART_FCR_CLEAR_XMIT |
            UART_FCR_CLEAR_RCVR | UART_FCR_ENABLE_FIFO;
        d2d_sync_writel(val, UART2_REG(UART_FCR));
        val = UART_MCR_OUT2 | UART_MCR_RTS | UART_MCR_DTR;
        d2d_sync_writel(val, UART2_REG(UART_MCR));

#if !defined(CONFIG_RHEA_D2D_LOOKBACK)
        /* IOMUX config */
        val = readl((void *) IOMUX_UART2_RX);
        val = (val & ~0x70) | 0x20;
        writel(val, (void *) IOMUX_UART2_RX);
        val = readl((void *) IOMUX_UART2_TX);
        val = (val & ~0x70) | 0x20;
        writel(val, (void *) IOMUX_UART2_TX);

        /* UART config */
        val = readl((void *) UART2_REG(UART_LCR));
        val |= UART_LCR_DLAB;
        writel(val, (void *) UART2_REG(UART_LCR));
        writel(UART_DLL_VAL, (void *) UART2_REG(UART_DLL));
        writel(UART_DLM_VAL, (void *) UART2_REG(UART_DLM));
        writel(UART_DLF_VAL, (void *) UART2_REG(DW_UART_DLF));
        val = readl((void *) UART2_REG(UART_LCR));
        val = (val & ~UART_LCR_DLAB) | UART_LCR_WLEN8;  // 115200 8N1
        writel(val, (void *) UART2_REG(UART_LCR));
        val = UART_FCR_R_TRIG_11 | UART_FCR_CLEAR_XMIT |
            UART_FCR_CLEAR_RCVR | UART_FCR_ENABLE_FIFO;
        writel(val, (void *) UART2_REG(UART_FCR));
        val = UART_MCR_OUT2 | UART_MCR_RTS | UART_MCR_DTR;
        writel(val, (void *) UART2_REG(UART_MCR));
    }
#endif

#if !defined(CONFIG_RHEA_D2D_LOOKBACK)
    if (CONFIG_RHEA_D2D_SELF_ID == RHEA_DIE0_ID) {
#endif
        // Send
        do {
            mdelay(10);
            if (timeout_ms++ > UART_THRE_TIMEOUT)
                return -EBUSY;
            d2d_sync_readl(&val, UART2_REG(UART_LSR));
        } while (!(val & UART_LSR_THRE));
        d2d_sync_writel(++i, UART2_REG(UART_TX));
#if !defined(CONFIG_RHEA_D2D_LOOKBACK)
    } else {
#endif
        // Receive
        do {
            mdelay(10);
            if (timeout_ms++ > UART_THRE_TIMEOUT)
                return -EBUSY;
            d2d_sync_readl(&val, UART2_REG(UART_LSR));
        } while (!(val & UART_LSR_DR));
        d2d_sync_readl(&val, UART2_REG(UART_RX));

        printf("=== %s Received: 0x%02x, ", __func__, val);
#if !defined(CONFIG_RHEA_D2D_LOOKBACK)
    }

    if (val == i) {
        printf("verification successful\n");
    } else {
        printf("expected 0x%02x, verification failed\n", i);
        return -EIO;
    }
#endif
    return 0;
}

static void d2d_irq_handler(uint8_t int_idx, uint32_t cmd, uint32_t data)
{
    printf("%s (int_idx=0x%x, cmd=0x%x, data=0x%x)\n",
        __func__, int_idx, cmd, data);
}

#define Timerx6_T1_IRQn (32)
#define TIMER_FREQ (25000000/100)
static volatile uint32_t timer_cmd = 0;
static volatile uint32_t timer_data = 0;
void timerx6_t1_irqhandler(void)
{
    uint8_t int_idx = timer_cmd % 4;
	(void)TIMERX6->Timer1EOI;
    printf("send interrupt(int_idx=0x%x, cmd=0x%x, data=0x%x)\n",
            int_idx, timer_cmd, timer_data);
	rhea_d2d_send_int2ap(RHEA_DIE0_IDX, 0, int_idx, 
                            timer_cmd++, timer_data--);
    if (timer_cmd > 5) timer_disable(Timerx6_T1);
}

static int d2d_cross_die_int_test(void)
{
	timer_init_config_t timer_init_config = {
		.int_mask = 0,
        .loadcount = 100 * TIMER_FREQ,
        .timer_id = Timerx6_T1,
        .timer_mode = Mode_User_Defined,
	};

    GIC_Init();
    if (CONFIG_RHEA_D2D_SELF_ID == RHEA_DIE0_ID) {
        rhea_d2d_irq_recv_enable(d2d_irq_handler);
    } else {
        timer_init(&timer_init_config);
        GIC_SetRouting(Timerx6_T1_IRQn, getAffinity(), 0);
        IRQ_SetHandler(Timerx6_T1_IRQn, timerx6_t1_irqhandler);
        GIC_SetPriority(Timerx6_T1_IRQn, 2 << 3);
        GIC_EnableIRQ(Timerx6_T1_IRQn);
        timer_enable(Timerx6_T1);
    }
    return 0;
}

int main(void)
{
    int ret = 0;
    unsigned int i;
    uint64_t start = 0;
    uint64_t elapsed = 0;
    systimer_id_t timer;

    printf("\n=== Self die id: %d\n", CONFIG_RHEA_D2D_SELF_ID);
    printf("=== Start die-to-die test ...\n");

    ret = rhea_d2d_init();
    if (ret)
        return ret;

    ret = rhea_d2d_sync_init();
    if (ret)
        goto err;

    ret = d2d_cross_die_int_test();
    if (ret)
        goto err;

    for (i = 0; i < 0; i++) {
        printf("\n=======================\n");
        printf("=== Test count: %03d ===\n", i);
        printf("=======================\n\n");

        ret = d2d_basic_func_test();
        if (ret)
            goto err;
        
        ret = d2d_sync_data_test();
        if (ret)
            goto err;

        ret = d2d_sync_reg_test(TEST_DATA_BUFL);
        if (ret)
            goto err;

        ret = d2d_sync_reg_test(TEST_NPU_ADDR);
        if (ret)
            goto err;

        // ret = d2d_sync_uart_test();
        // if (ret)
        //     goto err;
    }

    printf("\n=== die-to-die test done\n\n");

err:
    rhea_d2d_sync_exit();
    if (ret)
        printf("=== Obtain a return value of %d\n", ret);
    while (1);
    return 0;
}