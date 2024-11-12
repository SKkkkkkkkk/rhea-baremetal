#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <arch_features.h>

#include "d2d_test.h"
#include "d2d_api.h"
#include "d2d_sync.h"
#include "serial_reg.h"
#include "io.h"
#include "dw_apb_timers.h"
#include "gicv3.h"

static inline void delay(uint32_t value)
{
	volatile uint32_t i, j;

	for(i = 0; i < value; i++)
		for(j = 0; j < 1000; j++);
}

int verify_data(uint8_t *data, uint32_t size)
{
    uint32_t i;
    for (i = 0; i < size; i++) {
        if (data[i] != (i & 0xFF)) {
            printf("got %d at index %d, expect %d\n",
                    data[i], i, i & 0xFF);
            return -EIO;
        }
    }
    printf("data verify success\n");
    return 0;
}

int wait4flag_ap(uintptr_t addr, uint32_t flag)
{
    uint32_t timeout = 20;
    while (1) {
        if (readl((void *) addr) == flag) break;
        if (!timeout--) return -ETIMEDOUT;
        delay(1);
    }
    return 0;
}

int wait4flag_d2d(uintptr_t addr, uint32_t flag)
{
    uint32_t timeout = 20;
    uint32_t val;
    while (1) {
        rhea_d2d_readl(&val, addr);
        if (val == flag) break;
        if (!timeout--) return -ETIMEDOUT;
        delay(1);
    }
    return 0;
}

int die0_basic_rw_test(void)
{
	uint32_t val;

	delay(5);	// waiting for die1 ready

	printf("[%d]%s tile_id: %02x\n", __LINE__, __func__, 0x04);
	rhea_d2d_select_tile(RHEA_DIE1_IDX, 0x04, 0);
	rhea_d2d_writel(0xaa55aa55, 0x40000000);
	rhea_d2d_readl(&val, 0x40000000);
	printf("read 0x40000000 = 0x%x\n", val);
	rhea_d2d_writel(0x12345678, 0x40000010);
	rhea_d2d_readl(&val, 0x40000010);
	printf("read 0x40000010 = 0x%x\n", val);
	rhea_d2d_release_tile();
	printf("[%d]%s tile_id: %02x\n", __LINE__, __func__, 0x26);
	rhea_d2d_select_tile(RHEA_DIE1_IDX, 0x26, 0);
	rhea_d2d_writel(0xaa55aa55, 0x40000000);
	rhea_d2d_readl(&val, 0x40000000);
	printf("read 0x40000000 = 0x%x\n", val);
	rhea_d2d_writel(0x12345678, 0x40000010);
	rhea_d2d_readl(&val, 0x40000010);
	printf("read 0x40000010 = 0x%x\n", val);
	rhea_d2d_release_tile();
	printf("[%d]%s tile_id: %02x\n", __LINE__, __func__, 0x27);
	rhea_d2d_select_tile(RHEA_DIE1_IDX, 0x27, 0);
	rhea_d2d_writel(0xaa55aa55, 0x40000000);
	rhea_d2d_readl(&val, 0x40000000);
	printf("read 0x40000000 = 0x%x\n", val);
	rhea_d2d_writel(0x12345678, 0x40000010);
	rhea_d2d_readl(&val, 0x40000010);
	printf("read 0x40000010 = 0x%x\n", val);
	rhea_d2d_release_tile();
	printf("[%d]%s tile_id: %02x\n", __LINE__, __func__, 0x36);
	rhea_d2d_select_tile(RHEA_DIE1_IDX, 0x36, 0);
	rhea_d2d_writel(0xaa55aa55, 0x40000000);
	rhea_d2d_readl(&val, 0x40000000);
	printf("read 0x40000000 = 0x%x\n", val);
	rhea_d2d_writel(0x12345678, 0x40000010);
	rhea_d2d_readl(&val, 0x40000010);
	printf("read 0x40000010 = 0x%x\n", val);
	rhea_d2d_release_tile();
	printf("[%d]%s tile_id: %02x\n", __LINE__, __func__, 0x37);
	rhea_d2d_select_tile(RHEA_DIE1_IDX, 0x37, 0);
	rhea_d2d_writel(0xaa55aa55, 0x40000000);
	rhea_d2d_readl(&val, 0x40000000);
	printf("read 0x40000000 = 0x%x\n", val);
	rhea_d2d_writel(0x12345678, 0x40000010);
	rhea_d2d_readl(&val, 0x40000010);
	printf("read 0x40000010 = 0x%x\n", val);
	rhea_d2d_release_tile();
	return 0;
}

int die0_basic_memcpy_test(void)
{
#define TEST_DATA_SIZE  512
    int ret;
    uint8_t w_data[TEST_DATA_SIZE], r_data[TEST_DATA_SIZE];
    uint32_t val, i;

    for (i = 0; i < TEST_DATA_SIZE; i++) {
        w_data[i] = i & 0xFF;
    }

	rhea_d2d_select_tile(RHEA_DIE1_IDX, 0x04, 0);
    ret = wait4flag_d2d(0x40000000, 0); // waiting for die1 ready
    if (ret) return ret;

    memset(r_data, 0, TEST_DATA_SIZE);
	printf("[%d]%s tile_id: %02x\n", __LINE__, __func__, 0x04);
    rhea_d2d_write_data(w_data, 0x41000000, TEST_DATA_SIZE);
	rhea_d2d_writel(1, 0x40000000);
    rhea_d2d_read_data(r_data, 0x41000000, TEST_DATA_SIZE);
    ret = verify_data(r_data, TEST_DATA_SIZE);
    if (ret) return ret;
    ret = wait4flag_d2d(0x40000000, 0);
    if (ret) return ret;
	rhea_d2d_release_tile();

    memset(r_data, 0, TEST_DATA_SIZE);
	printf("[%d]%s tile_id: %02x\n", __LINE__, __func__, 0x26);
	rhea_d2d_select_tile(RHEA_DIE1_IDX, 0x26, 0);
    rhea_d2d_write_data(w_data, 0x41000000, TEST_DATA_SIZE);
	rhea_d2d_writel(1, 0x40000000);
    rhea_d2d_read_data(r_data, 0x41000000, TEST_DATA_SIZE);
    ret = verify_data(r_data, TEST_DATA_SIZE);
    if (ret) return ret;
    ret = wait4flag_d2d(0x40000000, 0);
    if (ret) return ret;
	rhea_d2d_release_tile();

    memset(r_data, 0, TEST_DATA_SIZE);
	printf("[%d]%s tile_id: %02x\n", __LINE__, __func__, 0x27);
	rhea_d2d_select_tile(RHEA_DIE1_IDX, 0x27, 0);
    rhea_d2d_write_data(w_data, 0x41000000, TEST_DATA_SIZE);
	rhea_d2d_writel(1, 0x40000000);
    rhea_d2d_read_data(r_data, 0x41000000, TEST_DATA_SIZE);
    ret = verify_data(r_data, TEST_DATA_SIZE);
    if (ret) return ret;
    ret = wait4flag_d2d(0x40000000, 0);
    if (ret) return ret;
	rhea_d2d_release_tile();

    memset(r_data, 0, TEST_DATA_SIZE);
	printf("[%d]%s tile_id: %02x\n", __LINE__, __func__, 0x36);
	rhea_d2d_select_tile(RHEA_DIE1_IDX, 0x36, 0);
    rhea_d2d_write_data(w_data, 0x41000000, TEST_DATA_SIZE);
	rhea_d2d_writel(1, 0x40000000);
    rhea_d2d_read_data(r_data, 0x41000000, TEST_DATA_SIZE);
    ret = verify_data(r_data, TEST_DATA_SIZE);
    if (ret) return ret;
    ret = wait4flag_d2d(0x40000000, 0);
    if (ret) return ret;
	rhea_d2d_release_tile();

    memset(r_data, 0, TEST_DATA_SIZE);
	printf("[%d]%s tile_id: %02x\n", __LINE__, __func__, 0x37);
	rhea_d2d_select_tile(RHEA_DIE1_IDX, 0x37, 0);
    rhea_d2d_write_data(w_data, 0x41000000, TEST_DATA_SIZE);
	rhea_d2d_writel(1, 0x40000000);
    rhea_d2d_read_data(r_data, 0x41000000, TEST_DATA_SIZE);
    ret = verify_data(r_data, TEST_DATA_SIZE);
    if (ret) return ret;
    ret = wait4flag_d2d(0x40000000, 0);
    if (ret) return ret;
	rhea_d2d_release_tile();
    return 0;
}

static int d2d_sync_writel(uint32_t val, uint32_t addr)
{
    int ret;
    struct d2d_sync_put_cmd put_cmd;

    put_cmd.die_idx = 1;
    put_cmd.cmd_id = D2D_SYNC_WRITEL;
    put_cmd.reg_addr = addr;
    put_cmd.reg_val = val;
    printf("%s 0x%x to 0x%x\n", __func__, val, addr);
    return d2d_sync_remote(&put_cmd);
}

static int d2d_sync_readl(uint32_t *val, uint32_t addr)
{
    int ret;
    struct d2d_sync_put_cmd put_cmd;

    put_cmd.die_idx = 1;
    put_cmd.cmd_id = D2D_SYNC_READL;
    put_cmd.reg_addr = addr;
    put_cmd.reg_val = 0;
    ret = d2d_sync_remote(&put_cmd);
    *val = put_cmd.reg_val;
    printf("%s 0x%x from 0x%x\n", __func__, *val, addr);
    return ret;
}

#define UART2_REG(x)        (0x10000000 + (x << 2))
#define UART_DLL_VAL        (0x06) /* clock: 20000000, baud rate: 115200 */
#define UART_DLM_VAL        (0x00)
#define UART_DLF_VAL        (0x08)
#define DW_UART_DLF	        (0x30) /* Divisor Latch Fraction Register */
#define UART_THRE_TIMEOUT   (20)
int die0_sync_reg_test(void)
{
    int ret;
    uint32_t val, timeout_ms = 0;

    printf("[%d]%s\n", __LINE__, __func__);
	rhea_d2d_select_tile(RHEA_DIE1_IDX, 0x04, 0);
    ret = wait4flag_d2d(0x40000000, 0); // waiting for die1 ready
    if (ret) return ret;
	rhea_d2d_release_tile();

    ret = rhea_d2d_sync_init();
    if (ret) return ret;

    // /* UART config */
    // d2d_sync_readl(&val, UART2_REG(UART_LCR));
    // val |= UART_LCR_DLAB;
    // d2d_sync_writel(val, UART2_REG(UART_LCR));
    // d2d_sync_writel(UART_DLL_VAL, UART2_REG(UART_DLL));
    // d2d_sync_writel(UART_DLM_VAL, UART2_REG(UART_DLM));
    // d2d_sync_writel(UART_DLF_VAL, UART2_REG(DW_UART_DLF));
    // d2d_sync_readl(&val, UART2_REG(UART_LCR));
    // val = (val & ~UART_LCR_DLAB) | UART_LCR_WLEN8;
    // d2d_sync_writel(val, UART2_REG(UART_LCR));
    // val = UART_FCR_R_TRIG_11 | UART_FCR_CLEAR_XMIT |
    //     UART_FCR_CLEAR_RCVR | UART_FCR_ENABLE_FIFO;
    // d2d_sync_writel(val, UART2_REG(UART_FCR));
    // val = UART_MCR_OUT2 | UART_MCR_RTS | UART_MCR_DTR;
    // d2d_sync_writel(val, UART2_REG(UART_MCR));

    // Send
    do {
        delay(1);
        if (timeout_ms++ > UART_THRE_TIMEOUT)
            return -ETIMEDOUT;
        d2d_sync_readl(&val, UART2_REG(UART_LSR));
    } while (!(val & UART_LSR_THRE));
    d2d_sync_writel('a', UART2_REG(UART_TX));
    d2d_sync_writel('\n', UART2_REG(UART_TX));

    rhea_d2d_sync_exit();

	rhea_d2d_select_tile(RHEA_DIE1_IDX, 0x04, 0);
	rhea_d2d_writel(1, 0x40000000);
	rhea_d2d_release_tile();
    return 0;
}

static void d2d_irq_handler(uint8_t int_idx, uint32_t cmd, uint32_t data)
{
    printf("%s (int_idx=0x%x, cmd=0x%x, data=0x%x)\n",
        __func__, int_idx, cmd, data);
}

int die0_interrupt_test(void)
{
    GIC_Init();
    rhea_d2d_irq_recv_enable(d2d_irq_handler);
    return 0;
}

int die0_d2d_switch_test(void)
{
    uint8_t tile_id;
    uint32_t val;
    uint64_t addr;
    printf("[%d]%s\n", __LINE__, __func__);

    for (int i = 0; i < 10; i++) {
        tile_id = 0x04;
        addr = 0x10050004;
        rhea_d2d_select_tile(RHEA_DIE1_IDX, tile_id, 0);
        rhea_d2d_writel(i, addr);
        rhea_d2d_readl(&val, addr);
        printf("[%d] tile %02x 0x%lx: 0x%x\n", i, tile_id, addr, val);
        rhea_d2d_release_tile();

        tile_id = 0x26;
        addr = 0x30300150;
        rhea_d2d_select_tile(RHEA_DIE1_IDX, tile_id, 0);
        rhea_d2d_writel(i, addr);
        rhea_d2d_readl(&val, addr);
        printf("[%d] tile %02x 0x%lx: 0x%x\n", i, tile_id, addr, val);
        rhea_d2d_release_tile();

        tile_id = 0x27;
        addr = 0x30300150;
        rhea_d2d_select_tile(RHEA_DIE1_IDX, tile_id, 0);
        rhea_d2d_writel(i, addr);
        rhea_d2d_readl(&val, addr);
        printf("[%d] tile %02x 0x%lx: 0x%x\n", i, tile_id, addr, val);
        rhea_d2d_release_tile();

        tile_id = 0x36;
        addr = 0x30300150;
        rhea_d2d_select_tile(RHEA_DIE1_IDX, tile_id, 0);
        rhea_d2d_writel(i, addr);
        rhea_d2d_readl(&val, addr);
        printf("[%d] tile %02x 0x%lx: 0x%x\n", i, tile_id, addr, val);
        rhea_d2d_release_tile();

        tile_id = 0x37;
        addr = 0x30300150;
        rhea_d2d_select_tile(RHEA_DIE1_IDX, tile_id, 0);
        rhea_d2d_writel(i, addr);
        rhea_d2d_readl(&val, addr);
        printf("[%d] tile %02x 0x%lx: 0x%x\n", i, tile_id, addr, val);
        rhea_d2d_release_tile();
    }
    return 0;
}

int run_die0_test(void)
{
    int ret;

    ret = die0_basic_rw_test();
    if (ret) {
        printf("[%d]%s error %d\n", __LINE__, __func__, ret);
        return ret;
    }
    ret = die0_basic_memcpy_test();
    if (ret) {
        printf("[%d]%s error %d\n", __LINE__, __func__, ret);
        return ret;
    }
    ret = die0_sync_reg_test();
    if (ret) {
        printf("[%d]%s error %d\n", __LINE__, __func__, ret);
        return ret;
    }
    ret = die0_interrupt_test();
    if (ret) {
        printf("[%d]%s error %d\n", __LINE__, __func__, ret);
        return ret;
    }
    return 0;
}

int die1_basic_rw_test(void)
{
    int ret = 0, i;
	uint32_t val;

	writel(0, (void *) (uintptr_t) 0x3800000208);
	writel(0, (void *) (uintptr_t) 0x0040000000);
	writel(0, (void *) (uintptr_t) 0x0040000010);
	writel(0, (void *) (uintptr_t) 0x2640000000);
	writel(0, (void *) (uintptr_t) 0x2640000010);
	writel(0, (void *) (uintptr_t) 0x2740000000);
	writel(0, (void *) (uintptr_t) 0x2740000010);
	writel(0, (void *) (uintptr_t) 0x3640000000);
	writel(0, (void *) (uintptr_t) 0x3640000010);
	writel(0, (void *) (uintptr_t) 0x3740000000);
	writel(0, (void *) (uintptr_t) 0x3740000010);

	for (i = 0; i < 5; i++) {
		val = readl((void *) (uintptr_t) 0x0040000000);
        if (val == 0xaa55aa55) ret++;
		printf("[%d]%s 0x0040000000: 0x%x\n", __LINE__, __func__, val);
		val = readl((void *) (uintptr_t) 0x0040000010);
        if (val == 0x12345678) ret++;
		printf("[%d]%s 0x0040000010: 0x%x\n", __LINE__, __func__, val);
		val = readl((void *) (uintptr_t) 0x2640000000);
        if (val == 0xaa55aa55) ret++;
		printf("[%d]%s 0x2640000000: 0x%x\n", __LINE__, __func__, val);
		val = readl((void *) (uintptr_t) 0x2640000010);
        if (val == 0x12345678) ret++;
		printf("[%d]%s 0x2640000010: 0x%x\n", __LINE__, __func__, val);
		val = readl((void *) (uintptr_t) 0x2740000000);
        if (val == 0xaa55aa55) ret++;
		printf("[%d]%s 0x2740000000: 0x%x\n", __LINE__, __func__, val);
		val = readl((void *) (uintptr_t) 0x2740000010);
        if (val == 0x12345678) ret++;
		printf("[%d]%s 0x2740000010: 0x%x\n", __LINE__, __func__, val);
		val = readl((void *) (uintptr_t) 0x3640000000);
        if (val == 0xaa55aa55) ret++;
		printf("[%d]%s 0x3640000000: 0x%x\n", __LINE__, __func__, val);
		val = readl((void *) (uintptr_t) 0x3640000010);
        if (val == 0x12345678) ret++;
		printf("[%d]%s 0x3640000010: 0x%x\n", __LINE__, __func__, val);
		val = readl((void *) (uintptr_t) 0x3740000000);
        if (val == 0xaa55aa55) ret++;
		printf("[%d]%s 0x3740000010: 0x%x\n", __LINE__, __func__, val);
		val = readl((void *) (uintptr_t) 0x3740000010);
        if (val == 0x12345678) ret++;
		printf("[%d]%s 0x3740000000: 0x%x\n", __LINE__, __func__, val);
        if (ret == 10) return 0;
	}
	return -ETIMEDOUT;
}

int die1_basic_memcpy_test(void)
{
#define TEST_DATA_SIZE  512
    int ret;
    uint8_t *r_data;
    uint32_t val, i;

    writel(0, (void *) 0x0040000000);
    writel(0, (void *) 0x2640000000);
    writel(0, (void *) 0x2740000000);
    writel(0, (void *) 0x3640000000);
    writel(0, (void *) 0x3740000000);

	printf("[%d]%s tile_id: %02x\n", __LINE__, __func__, 0x04);
    r_data = (uint8_t *) 0x0041000000;
    memset(r_data, 0, TEST_DATA_SIZE);
    ret = wait4flag_ap(0x0040000000, 1);
    if (ret) return ret;
    ret = verify_data(r_data, TEST_DATA_SIZE);
    if (ret) return ret;
    writel(0, (void *) 0x0040000000);

	printf("[%d]%s tile_id: %02x\n", __LINE__, __func__, 0x26);
    r_data = (uint8_t *) 0x2641000000;
    memset(r_data, 0, TEST_DATA_SIZE);
    ret = wait4flag_ap(0x2640000000, 1);
    if (ret) return ret;
    ret = verify_data(r_data, TEST_DATA_SIZE);
    if (ret) return ret;
    writel(0, (void *) 0x2640000000);

	printf("[%d]%s tile_id: %02x\n", __LINE__, __func__, 0x27);
    r_data = (uint8_t *) 0x2741000000;
    memset(r_data, 0, TEST_DATA_SIZE);
    ret = wait4flag_ap(0x2740000000, 1);
    if (ret) return ret;
    ret = verify_data(r_data, TEST_DATA_SIZE);
    if (ret) return ret;
    writel(0, (void *) 0x2740000000);

	printf("[%d]%s tile_id: %02x\n", __LINE__, __func__, 0x36);
    r_data = (uint8_t *) 0x3641000000;
    memset(r_data, 0, TEST_DATA_SIZE);
    ret = wait4flag_ap(0x3640000000, 1);
    if (ret) return ret;
    ret = verify_data(r_data, TEST_DATA_SIZE);
    if (ret) return ret;
    writel(0, (void *) 0x3640000000);

	printf("[%d]%s tile_id: %02x\n", __LINE__, __func__, 0x37);
    r_data = (uint8_t *) 0x3741000000;
    memset(r_data, 0, TEST_DATA_SIZE);
    ret = wait4flag_ap(0x3740000000, 1);
    if (ret) return ret;
    ret = verify_data(r_data, TEST_DATA_SIZE);
    if (ret) return ret;
    writel(0, (void *) 0x3740000000);
    return 0;
}

int die1_sync_reg_test(void)
{
    int ret;

    printf("[%d]%s\n", __LINE__, __func__);
	writel(0, (void *) (uintptr_t) 0x0040000000);

    ret = rhea_d2d_sync_init();
    if (ret) return ret;

    while (1) {
        ret = d2d_sync_obtain_cmd();
        if (ret < 0) return ret;
        else if (readl((void *) 0x0040000000) == 1) break;
        else if (ret == 0) continue;
    }
    
    rhea_d2d_sync_exit();
    return 0;
}

static volatile uint32_t timer_cmd = 0;
static volatile uint32_t timer_data = 0;
int die1_interrupt_test(void)
{
    uint8_t int_idx = timer_cmd % 4;

    while (1) {
        printf("send interrupt(int_idx=0x%x, cmd=0x%x, data=0x%x)\n",
                int_idx, timer_cmd, timer_data);
        rhea_d2d_send_int2ap(RHEA_DIE0_IDX, 0, int_idx, 
                                timer_cmd++, timer_data--);
        delay(1);
        if (timer_cmd > 4) break;
    }
    return 0;
}

int run_die1_test(void)
{
    int ret;

    ret = die1_basic_rw_test();
    if (ret) {
        printf("[%d]%s error %d\n", __LINE__, __func__, ret);
        return ret;
    }
    ret = die1_basic_memcpy_test();
    if (ret) {
        printf("[%d]%s error %d\n", __LINE__, __func__, ret);
        return ret;
    }
    ret = die1_sync_reg_test();
    if (ret) {
        printf("[%d]%s error %d\n", __LINE__, __func__, ret);
        return ret;
    }
    ret = die1_interrupt_test();
    if (ret) {
        printf("[%d]%s error %d\n", __LINE__, __func__, ret);
        return ret;
    }
    return 0;
}
