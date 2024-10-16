#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "d2d_api.h"
#include "delay.h"
#include "memmap.h"
#include "gicv3.h"

#define D2D_DNOC_BASE           (0x3800000000)
#define D2D_CNOC_BASE           (0x9C00000000)
#define D2D_SYS_SIZE            (0x100000000)
#define D2D_DIE_OFFSET(die)     ((die) * 0x1000)
#define D2D_DIE_MAX             (4)
#define D2D_CFG_REG_MASK        (D2D_DIE_OFFSET(D2D_DIE_MAX) - 1)
/* CLCI bridge working mode */
#define D2D_REG_WORK_MODE       (0x0200)
#define D2D_WORK_MODE0          (0x0)   /* 2 die normal mode */
#define D2D_WORK_MODE1          (0x1)   /* 2 die retention mode */
#define D2D_WORK_MODE2          (0x2)   /* 4 die normal mode */
#define D2D_WORK_MODE3          (0x3)   /* 4 die retention mode */
#define D2D_WORK_MODE_MASK      (0x3)
/* Destination tile ID of the data received by CLCI */
#define D2D_REG_CFG_TILE_ID     (0x0208)
/* D2D lock */
#define D2D_REG_LOCK_BASE       (0x0800)
#define D2D_REG_LOCK(idx)       (D2D_REG_LOCK_BASE + (((idx) & 0xf) << 7))

#define D2D_CLCI0_APB_BASE      (0x20040000)
#define D2D_REG_CLCI0_CH_UP     (D2D_CLCI0_APB_BASE + 0x000)
#define D2D_CLCI2_APB_BASE      (0x20080000)
#define D2D_REG_CLCI2_CH_UP     (D2D_CLCI2_APB_BASE + 0x000)
/* Mailbox interrupt */
#define MAILBOX_IRQn            (32 + 27)
#define MAILBOX_REG_B2A_INTEN   ((uintptr_t) (MAILBOX_BASE + 0x28))
#define MAILBOX_INT_EN0         (1 << 0)
#define MAILBOX_INT_EN1         (1 << 1)
#define MAILBOX_INT_EN2         (1 << 2)
#define MAILBOX_INT_EN3         (1 << 3)
#define MAILBOX_REG_B2A_STATUS  ((uintptr_t) (MAILBOX_BASE + 0x2C))
#define MAILBOX_CLEAR_INT0      (1 << 0)
#define MAILBOX_CLEAR_INT1      (1 << 1)
#define MAILBOX_CLEAR_INT2      (1 << 2)
#define MAILBOX_CLEAR_INT3      (1 << 3)
#define MAILBOX_REG_B2A_CMD0    ((uintptr_t) (MAILBOX_BASE + 0x30))
#define MAILBOX_REG_B2A_DAT0    ((uintptr_t) (MAILBOX_BASE + 0x34))

#define AP_SYS_TILE_ID          (0x04)

static void *d2d_dnoc = NULL;
static void *d2d_cnoc = NULL;

static inline void rhea_d2d_cfg_writel(uint8_t die, uint32_t val, uint32_t offset)
{
    void *ioaddr = (die == RHEA_DIE_SELF) ? d2d_cnoc : d2d_dnoc;
    writel(val, ioaddr + D2D_DIE_OFFSET(die) + offset);
}

static inline uint32_t rhea_d2d_cfg_readl(uint8_t die, uint32_t offset)
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

static void (*d2d_irqhandler)(uint32_t cmd, uint32_t data);
static void __d2d_irqhandler(void)
{
    d2d_irqhandler(readl((void *) MAILBOX_REG_B2A_CMD0),
                    readl((void *) MAILBOX_REG_B2A_DAT0));
    writel(MAILBOX_CLEAR_INT0, (void *) MAILBOX_REG_B2A_STATUS);
}

void rhea_d2d_irq_recv_enable(void (*handler)(uint32_t cmd, uint32_t data))
{
    assert(handler != NULL);

    d2d_irqhandler = handler;
	GIC_SetRouting(MAILBOX_IRQn, getAffinity(), 0);
	IRQ_SetHandler(MAILBOX_IRQn, __d2d_irqhandler);
	GIC_SetPriority(MAILBOX_IRQn, 2 << 3);
	GIC_EnableIRQ(MAILBOX_IRQn);
}

int rhea_d2d_send_interrupt(uint8_t die, uint32_t timeout_ms,
                                uint32_t cmd, uint32_t data)
{
    int ret;
    pr_dbg("Send interrupt to die%d with cmd(0x%x) and data(0x%x)\n", 
            RHEA_DIE_IDX2ID(die), cmd, data);

    ret = rhea_d2d_select_tile(die, AP_SYS_TILE_ID, timeout_ms);
    if (ret) return ret;
    rhea_d2d_writel(MAILBOX_INT_EN0, MAILBOX_REG_B2A_INTEN);
    rhea_d2d_writel(cmd, MAILBOX_REG_B2A_CMD0);
    rhea_d2d_writel(data, MAILBOX_REG_B2A_DAT0);
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

int rhea_d2d_init(void)
{
    d2d_dnoc = (void *) D2D_DNOC_BASE;
    d2d_cnoc = (void *) D2D_CNOC_BASE;
    if (CONFIG_RHEA_D2D_WORK_MODE & ~D2D_WORK_MODE_MASK) {
        printf("Unsupported mode %d\n", CONFIG_RHEA_D2D_WORK_MODE);
        return -EINVAL;
    }
    rhea_d2d_cfg_writel(RHEA_DIE_SELF, 
                CONFIG_RHEA_D2D_WORK_MODE, D2D_REG_WORK_MODE);

    if (!rhea_d2d_cfg_readl(RHEA_DIE_SELF, D2D_REG_CLCI0_CH_UP) ||
        !rhea_d2d_cfg_readl(RHEA_DIE_SELF, D2D_REG_CLCI2_CH_UP))
        return -EFAULT;

    printf("Working in mode %d\n", CONFIG_RHEA_D2D_WORK_MODE);

    return 0;
}