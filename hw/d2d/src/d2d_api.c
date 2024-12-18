#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "d2d_api.h"
#include "delay.h"
#include "memmap.h"
#include "gicv3.h"
#include "io.h"

#define D2D_DNOC_BASE           (0x3800000000)
#define D2D_CNOC_BASE           (0x9C00000000)
#define D2D_SYS_SIZE            (0x100000000)
#define D2D_DIE_OFFSET(die)     ((die) * 0x1000)
#define D2D_DIE_MAX             (4)
#define D2D_CFG_REG_MASK        (D2D_DIE_OFFSET(D2D_DIE_MAX) - 1)
/* CLCIx reset */
#define D2D_REG_CLCIx_RESET(x)  (0x0100 + (x) * 0x4)
#define D2D_REG_MBI_TX_RESET    (0x0110)
#define D2D_REG_CLCI_BRIDGE_RESET   (0x0114)
#define D2D_REG_CLCIx_MCU_RESET(x)  (0x0118 + (x) * 0x4)
/* CLCI bridge working mode */
#define D2D_REG_CFG_WORK_MODE   (0x0200)
#define D2D_WORK_MODE0          (0x0)   /* 2 die normal mode */
#define D2D_WORK_MODE1          (0x1)   /* 2 die retention mode */
#define D2D_WORK_MODE2          (0x2)   /* 4 die normal mode */
#define D2D_WORK_MODE3          (0x3)   /* 4 die retention mode */
#define D2D_WORK_MODE_MASK      (0x3)
#define D2D_REG_CFG_POSTW_EN    (0x0204)
/* Destination tile ID of the data received by CLCI */
#define D2D_REG_CFG_TILE_ID     (0x0208)
#define D2D_REG_CFG_RX_SEL_GRP0 (0x020C)
#define D2D_REG_CFG_RX_SEL_GRP1 (0x0210)
#define D2D_REG_CFG_TX_SEL_GRP0 (0x0214)
#define D2D_REG_CFG_TX_SEL_GRP1 (0x0218)
#define D2D_REG_CLCIx_APB_MUX(x)    (0x0300 + (x) * 0x0100)
#define D2D_REG_CLCIx_CHIPLET_ID(x) (0x0304 + (x) * 0x0100)
#define D2D_REG_CLCIx_I2C_ID(x)     (0x0308 + (x) * 0x0100)
#define D2D_REG_CLCIx_BOOT_SEL(x)   (0x0314 + (x) * 0x0100)
#define D2D_REG_CFG_DNIU_POSTW  (0x0704)

#define CLCI_BOOT_SEL_UCIE      (0x0)
#define CLCI_BOOT_SEL_UART      (0x2)
/* D2D lock */
#define D2D_REG_LOCK_BASE       (0x0800)
#define D2D_REG_LOCK(idx)       (D2D_REG_LOCK_BASE + (((idx) & 0xf) << 7))
/* CLCIx_APB (CLCI_BIU) */
#define CLCIx_APB_BASE(x)       (0x20040000 + (x) * 0x20000)
/* CLCIx_AHB */
#define CLCIx_AHB_BASE(x)       (0x20200000 + (x) * 0x200000)
#define CLCI_MCU_ROM_OFFSET     (0x00000)
#define CLCI_MCU_RAM_OFFSET     (0x10000)
#define CLCI_MCU_LOCAL_CTRL_OFFSET  (0x20000)
#define CLCI_MCU_BIU_OFFSET     (0x30000)
#define CLCIx_AHB_SET_ROLE(x)   (CLCIx_AHB_BASE(x) + CLCI_MCU_RAM_OFFSET + 0x7FF4)
#define CLCIx_MCU_BIU_REG(x, reg)   (CLCIx_APB_BASE(x) + CLCI_MCU_BIU_OFFSET + CLCI_REG_##reg)

/* Sysctrl CLCI about */
/* Clock */
#define SYSCTRL_CLCI_CLK_EN1    ((uintptr_t) (SYSCTRL_BASE + 0x104))
#define SYSCTRL_CLCI_PLL_DIV    ((uintptr_t) (SYSCTRL_BASE + 0x1D8))
#define SYSCTRL_CLCI_CFG_DIV    ((uintptr_t) (SYSCTRL_BASE + 0x1DC))
#define SYSCTRL_CLCI_AXI_DIV    ((uintptr_t) (SYSCTRL_BASE + 0x1E0))
#define SYSCTRL_CLCI_SCAN_20_DIV    ((uintptr_t) (SYSCTRL_BASE + 0x1E4))
#define SYSCTRL_CLCI_SCAN_80_DIV    ((uintptr_t) (SYSCTRL_BASE + 0x1E8))
#define SYSCTRL_CLCI_MCU_DIV    ((uintptr_t) (SYSCTRL_BASE + 0x1F4))
/* Pinmux */
#define SYSCTRL_CLCIx_UART_RX(x)    ((uintptr_t) (SYSCTRL_BASE + 0x8C8 + (x) * 0x8))
#define SYSCTRL_CLCIx_UART_TX(x)    ((uintptr_t) (SYSCTRL_BASE + 0x8CC + (x) * 0x8))

/* Mailbox interrupt */
#define MAILBOX_IRQn            (32 + 27)
#define MAILBOX_REG_A2B_INTEN   ((uintptr_t) (MAILBOX_BASE + 0x00))
#define MAILBOX_REG_A2B_STATUS  ((uintptr_t) (MAILBOX_BASE + 0x04))
#define MAILBOX_REG_A2B_CMD(x)  ((uintptr_t) (MAILBOX_BASE + 0x08 + ((x) * 0x8)))
#define MAILBOX_REG_A2B_DAT(x)  ((uintptr_t) (MAILBOX_BASE + 0x0C + ((x) * 0x8)))
#define MAILBOX_REG_B2A_INTEN   ((uintptr_t) (MAILBOX_BASE + 0x28))
#define MAILBOX_REG_B2A_STATUS  ((uintptr_t) (MAILBOX_BASE + 0x2C))
#define MAILBOX_REG_B2A_CMD(x)  ((uintptr_t) (MAILBOX_BASE + 0x30 + ((x) * 0x8)))
#define MAILBOX_REG_B2A_DAT(x)  ((uintptr_t) (MAILBOX_BASE + 0x34 + ((x) * 0x8)))
#define MAILBOX_INT_EN(x)       (1 << (x))
#define MAILBOX_CLEAR_INT(x)    (1 << (x))
#define MAILBOX_INT_IDX_MAX     (4)

#define AP_SYS_TILE_ID          (0x04)

enum clci_idx {
    CLCI0,
    CLCI1,
    CLCI2,
    CLCI3,

    CLCI_MAX,
};

static void *d2d_dnoc = NULL;
static void *d2d_cnoc = NULL;

void rhea_d2d_cfg_writel(uint8_t die, uint32_t val, uint32_t offset)
{
    void *ioaddr = (die == RHEA_DIE_SELF) ? d2d_cnoc : d2d_dnoc;
    writel(val, ioaddr + D2D_DIE_OFFSET(die) + offset);
}

uint32_t rhea_d2d_cfg_readl(uint8_t die, uint32_t offset)
{
    void *ioaddr = (die == RHEA_DIE_SELF) ? d2d_cnoc : d2d_dnoc;
    return readl(ioaddr + D2D_DIE_OFFSET(die) + offset);
}

int rhea_d2d_write_data(const void *src, uint64_t addr, uint32_t count)
{
    if (!(addr & ~D2D_CFG_REG_MASK)) {
        printf("The address 0x%lx write is invalid\n", 
                addr);
        return -EFAULT;
    }
    memcpy(d2d_dnoc + addr, src, count);
    return 0;
}

int rhea_d2d_read_data(void *dst, uint64_t addr, uint32_t count)
{
    if (!(addr & ~D2D_CFG_REG_MASK)) {
        printf("The address 0x%lx read is invalid\n", 
                addr);
        return -EFAULT;
    }
    memcpy(dst, d2d_dnoc + addr, count);
    return 0;
}

int rhea_d2d_writeb(uint8_t val, uint64_t addr)
{
    if (!(addr & ~D2D_CFG_REG_MASK)) {
        printf("The address 0x%lx written is invalid\n", 
                addr);
        return -EFAULT;
    }
    writeb(val, d2d_dnoc + addr);
    return 0;
}

int rhea_d2d_readb(uint8_t *val, uint64_t addr)
{
    if (!(addr & ~D2D_CFG_REG_MASK)) {
        printf("The address 0x%lx read is invalid\n", 
                addr);
        return -EFAULT;
    }
    *val = readb(d2d_dnoc + addr);
    return 0;
}

int rhea_d2d_writew(uint16_t val, uint64_t addr)
{
    if (!(addr & ~D2D_CFG_REG_MASK)) {
        printf("The address 0x%lx written is invalid\n", 
                addr);
        return -EFAULT;
    }
    writew(val, d2d_dnoc + addr);
    return 0;
}

int rhea_d2d_readw(uint16_t *val, uint64_t addr)
{
    if (!(addr & ~D2D_CFG_REG_MASK)) {
        printf("The address 0x%lx read is invalid\n", 
                addr);
        return -EFAULT;
    }
    *val = readw(d2d_dnoc + addr);
    return 0;
}

int rhea_d2d_writel(uint32_t val, uint64_t addr)
{
    if (!(addr & ~D2D_CFG_REG_MASK)) {
        printf("The address 0x%lx written is invalid\n", 
                addr);
        return -EFAULT;
    }
    writel(val, d2d_dnoc + addr);
    return 0;
}

int rhea_d2d_readl(uint32_t *val, uint64_t addr)
{
    if (!(addr & ~D2D_CFG_REG_MASK)) {
        printf("The address 0x%lx read is invalid\n", 
                addr);
        return -EFAULT;
    }
    *val = readl(d2d_dnoc + addr);
    return 0;
}

int rhea_d2d_writeq(uint64_t val, uint64_t addr)
{
    if (!(addr & ~D2D_CFG_REG_MASK)) {
        printf("The address 0x%lx written is invalid\n", 
                addr);
        return -EFAULT;
    }
    writeq(val, d2d_dnoc + addr);
    return 0;
}

int rhea_d2d_readq(uint64_t *val, uint64_t addr)
{
    if (!(addr & ~D2D_CFG_REG_MASK)) {
        printf("The address 0x%lx read is invalid\n", 
                addr);
        return -EFAULT;
    }
    *val = readq(d2d_dnoc + addr);
    return 0;
}

int rhea_d2d_get_lock(uint8_t die, enum rhea_d2d_lock idx)
{
    if (idx >= D2D_LOCK_MAX) {
        printf("The selected lock%d does not exist\n", idx);
        return -EINVAL;
    }
    return rhea_d2d_cfg_readl(die, D2D_REG_LOCK(idx));
}

void rhea_d2d_release_lock(uint8_t die, enum rhea_d2d_lock idx)
{
    if (idx >= D2D_LOCK_MAX) {
        printf("The selected lock%d does not exist\n", idx);
        return;
    }

    rhea_d2d_cfg_writel(die, 0x1, D2D_REG_LOCK(idx));
}

int rhea_d2d_select_tile(uint8_t die, uint8_t tile_id, uint32_t wait_ms)
{
    uint8_t i;

    if (die >= CONFIG_RHEA_DIE_MAX) {
        printf("die%d does not exist\n", die);
        return -EINVAL;
    }

    for (i = 0; i <= die; i++) {
        /* TODO: Lock each die in the data transfer path */
        while (i == die && !rhea_d2d_get_lock(i, D2D_LOCK_TILE_SEL)) {
            if (wait_ms) {
                wait_ms--;
                mdelay(1);
                continue;
            }
            printf("die%d is busy\n", i);
            return -EBUSY;
        }
        /* Except for the target die, the tile IDs on the remaining paths
           are configured to pass through data to the next die (set to 0) */
        rhea_d2d_cfg_writel(i, i == die ? tile_id : 0, 
                                D2D_REG_CFG_TILE_ID);
    }

    return 0;
}

int rhea_d2d_get_selected_tile(uint8_t die, uint8_t *tile_id)
{
    if (die >= CONFIG_RHEA_DIE_MAX) {
        printf("die%d does not exist\n", die);
        return -EINVAL;
    }

    *tile_id = rhea_d2d_cfg_readl(die, D2D_REG_CFG_TILE_ID);
    return 0;
}

void rhea_d2d_release_tile(void)
{
    uint8_t i;
    for (i = 0; i < CONFIG_RHEA_DIE_MAX; i++) {
        rhea_d2d_cfg_writel(i, 0, D2D_REG_CFG_TILE_ID);
        rhea_d2d_release_lock(i, D2D_LOCK_TILE_SEL);
    }
}

static inline int find_bit(uint8_t val)
{
    for (uint8_t i = 0; i < MAILBOX_INT_IDX_MAX; i++) {
        if (val & (1 << i)) {
            return i;
        }
    }
    return -1;
}

static d2d_irq_handler_t d2d_irq_handler;
static void __d2d_irqhandler(void)
{
    int8_t int_idx;
    
    int_idx = readl((void *) MAILBOX_REG_B2A_STATUS);
    int_idx = find_bit(int_idx);
    assert(int_idx >= 0);
    d2d_irq_handler(int_idx, readl((void *) MAILBOX_REG_B2A_CMD(int_idx)),
                    readl((void *) MAILBOX_REG_B2A_DAT(int_idx)));
    writel(MAILBOX_CLEAR_INT(int_idx), (void *) MAILBOX_REG_B2A_STATUS);
}

void rhea_d2d_irq_recv_enable(d2d_irq_handler_t handler)
{
    assert(handler != NULL);

    d2d_irq_handler = handler;
	GIC_SetRouting(MAILBOX_IRQn, getAffinity(), 0);
	IRQ_SetHandler(MAILBOX_IRQn, __d2d_irqhandler);
	GIC_SetPriority(MAILBOX_IRQn, 2 << 3);
	GIC_EnableIRQ(MAILBOX_IRQn);
}

int rhea_d2d_send_int2ap(uint8_t die, uint32_t timeout_ms,
                        uint8_t int_idx, uint32_t cmd, uint32_t data)
{
    int ret;

    assert(int_idx < MAILBOX_INT_IDX_MAX);
    pr_dbg("Send int%d to AP in die%d with cmd(0x%x) and data(0x%x)\n", 
            int_idx, RHEA_DIE_IDX2ID(die), cmd, data);

    ret = rhea_d2d_select_tile(die, AP_SYS_TILE_ID, timeout_ms);
    if (ret) return ret;
    rhea_d2d_writel(MAILBOX_INT_EN(int_idx), MAILBOX_REG_B2A_INTEN);
    rhea_d2d_writel(cmd, MAILBOX_REG_B2A_CMD(int_idx));
    rhea_d2d_writel(data, MAILBOX_REG_B2A_DAT(int_idx));
    rhea_d2d_release_tile();
    return ret;
}

int rhea_d2d_send_int2pcie(uint32_t timeout_ms, uint32_t cmd, 
                            uint8_t int_idx, uint32_t data)
{
    int ret;

    assert(int_idx < MAILBOX_INT_IDX_MAX);
    pr_dbg("Send int%d to PCIE with cmd(0x%x) and data(0x%x)\n", 
            int_idx, cmd, data);

    ret = rhea_d2d_select_tile(RHEA_DIE0_IDX, AP_SYS_TILE_ID, timeout_ms);
    if (ret) return ret;
    rhea_d2d_writel(MAILBOX_INT_EN(int_idx), MAILBOX_REG_A2B_INTEN);
    rhea_d2d_writel(cmd, MAILBOX_REG_A2B_CMD(int_idx));
    rhea_d2d_writel(data, MAILBOX_REG_A2B_DAT(int_idx));
    rhea_d2d_release_tile();
    return ret;
}

void *rhea_d2d_get_dnoc_addr(void)
{
    return d2d_dnoc;
}

void *rhea_d2d_get_cnoc_addr(void)
{
    return d2d_cnoc;
}

static void rhea_clci_clk_init(void)
{
    uint32_t tmp_val;

    /* Sysctrl configuration */
    tmp_val = readl((void *) SYSCTRL_CLCI_CLK_EN1);
    tmp_val |= 0xffcc400;
    writel(tmp_val, (void *) SYSCTRL_CLCI_CLK_EN1);
    writel(1, (void *) SYSCTRL_CLCI_PLL_DIV);
    writel(1, (void *) SYSCTRL_CLCI_CFG_DIV);
    writel(1, (void *) SYSCTRL_CLCI_AXI_DIV);
    writel(1, (void *) SYSCTRL_CLCI_SCAN_20_DIV);
    writel(1, (void *) SYSCTRL_CLCI_SCAN_80_DIV);
    writel(1, (void *) SYSCTRL_CLCI_MCU_DIV);
}

static void rhea_clci_pinmux_init(void)
{
    uint32_t val;
    enum clci_idx clci_idx;

    for (clci_idx = CLCI0; clci_idx < CLCI_MAX; clci_idx++) {
        val = readl((void *) SYSCTRL_CLCIx_UART_RX(clci_idx));
        val &= ~0x70;
        writel(val, (void *) SYSCTRL_CLCIx_UART_RX(clci_idx));
        val = readl((void *) SYSCTRL_CLCIx_UART_TX(clci_idx));
        val &= ~0x70;
        writel(val, (void *) SYSCTRL_CLCIx_UART_TX(clci_idx));
    }
}

int rhea_d2d_init(void)
{
    enum clci_idx clci_idx;
    uint32_t timeout = 10000;
    uint32_t tmp_val;

    d2d_dnoc = (void *) D2D_DNOC_BASE;
    d2d_cnoc = (void *) D2D_CNOC_BASE;

    /* CLCI configuration */
    rhea_clci_clk_init();
    rhea_clci_pinmux_init();
    for (clci_idx = CLCI0; clci_idx < CLCI_MAX; clci_idx++) {
        rhea_d2d_cfg_writel(RHEA_DIE_SELF, 0,
                            D2D_REG_CLCIx_APB_MUX(clci_idx));
        rhea_d2d_cfg_writel(RHEA_DIE_SELF, clci_idx,
                            D2D_REG_CLCIx_CHIPLET_ID(clci_idx));
        rhea_d2d_cfg_writel(RHEA_DIE_SELF, clci_idx,
                            D2D_REG_CLCIx_I2C_ID(clci_idx));
        rhea_d2d_cfg_writel(RHEA_DIE_SELF, CLCI_BOOT_SEL_UART,
                            D2D_REG_CLCIx_BOOT_SEL(clci_idx));
    }
    printf("CLCI configuration done\n");

    /* CLCI auto link */
    printf("CLCI waiting for link\n");
    for (clci_idx = CLCI0; clci_idx < CLCI_MAX; clci_idx++) {
        while (1) {
            tmp_val = rhea_d2d_cfg_readl(RHEA_DIE_SELF,
                            CLCIx_AHB_BASE(clci_idx) + 0x2105c);
            if ((tmp_val & 0xf) == 0x2) break;
            if (!timeout--) {
                printf("clci%d init timed out\n", clci_idx);
                goto timeout;
            }
            udelay(1000);
        }
        pr_dbg("clci%d init done\n", clci_idx);
        while (1) {
            tmp_val = rhea_d2d_cfg_readl(RHEA_DIE_SELF,
                            CLCIx_AHB_BASE(clci_idx) + 0x3003c);
            if ((tmp_val & 0x00001c00) == 0x00001c00) break;
            if (!timeout--) {
                printf("clci%d link timed out\n", clci_idx);
                goto timeout;
            }
            udelay(1000);
        }
        pr_dbg("clci%d link success\n", clci_idx);
        rhea_d2d_cfg_writel(RHEA_DIE_SELF, 0xa0,
                            CLCIx_AHB_BASE(clci_idx) + 0x3021c);
        rhea_d2d_cfg_writel(RHEA_DIE_SELF, 0xff0080,
                            CLCIx_AHB_BASE(clci_idx) + 0x30304);
    }
    printf("CLCI link finished\n");

    /* D2D configuration */
    rhea_d2d_cfg_writel(RHEA_DIE_SELF, 0x0, D2D_REG_CFG_WORK_MODE);
    rhea_d2d_cfg_writel(RHEA_DIE_SELF, 0x1, D2D_REG_CFG_POSTW_EN);
    rhea_d2d_cfg_writel(RHEA_DIE_SELF, 0x0, D2D_REG_CFG_RX_SEL_GRP0);
    rhea_d2d_cfg_writel(RHEA_DIE_SELF, 0x1, D2D_REG_CFG_RX_SEL_GRP1);
    rhea_d2d_cfg_writel(RHEA_DIE_SELF, 0x0, D2D_REG_CFG_TX_SEL_GRP0);
    rhea_d2d_cfg_writel(RHEA_DIE_SELF, 0x1, D2D_REG_CFG_TX_SEL_GRP1);
    rhea_d2d_cfg_writel(RHEA_DIE_SELF, 0x1, D2D_REG_CFG_DNIU_POSTW);
    printf("D2D configuration done\n");
    return 0;

timeout:
    printf("reg dump (0x2105c:0x%x, 0x21054:0x%x, 0x30004:0x%x, 0x17c1c:0x%x, 0x17c24:0x%x, 0x17c30:0x%x)\n",
        rhea_d2d_cfg_readl(RHEA_DIE_SELF, CLCIx_AHB_BASE(clci_idx) + 0x2105c),
        rhea_d2d_cfg_readl(RHEA_DIE_SELF, CLCIx_AHB_BASE(clci_idx) + 0x21054),
        rhea_d2d_cfg_readl(RHEA_DIE_SELF, CLCIx_AHB_BASE(clci_idx) + 0x30004),
        rhea_d2d_cfg_readl(RHEA_DIE_SELF, CLCIx_AHB_BASE(clci_idx) + 0x17c1c),
        rhea_d2d_cfg_readl(RHEA_DIE_SELF, CLCIx_AHB_BASE(clci_idx) + 0x17c24),
        rhea_d2d_cfg_readl(RHEA_DIE_SELF, CLCIx_AHB_BASE(clci_idx) + 0x17c30));
    return -ETIMEDOUT;
}