#ifndef __D2D_API_H__
#define __D2D_API_H__

#include <stdint.h>
#include <stdio.h>

#include "clci_mcu_reg.h"
#include "commands_sys.h"

#if defined(DEBUG)
#define pr_dbg(fmt, ...)    printf(fmt, __VA_ARGS__)
#else
#define pr_dbg(fmt, ...)    do {} while (0)
#endif

/* Physical ID (the die connected to PCIE is die0) */
enum rhea_die_id {
    RHEA_DIE0_ID = 0,
#if CONFIG_RHEA_DIE_MAX > 1
    RHEA_DIE1_ID,
#if CONFIG_RHEA_DIE_MAX > 2
    RHEA_DIE2_ID,
    RHEA_DIE3_ID,
#endif /* CONFIG_RHEA_DIE_MAX > 2 */
#endif /* CONFIG_RHEA_DIE_MAX > 1 */
    
    RHEA_DIE_ID_MAX = CONFIG_RHEA_DIE_MAX,
};

#define RHEA_DIE_ID2IDX(id) \
    (((id) + CONFIG_RHEA_DIE_MAX - CONFIG_RHEA_D2D_SELF_ID) % CONFIG_RHEA_DIE_MAX)
#define RHEA_DIE_IDX2ID(idx) \
    (((idx) + CONFIG_RHEA_D2D_SELF_ID) % CONFIG_RHEA_DIE_MAX)
/* Relative index (used to operate cfg register) */
#define RHEA_DIE_SELF           0
#define RHEA_DIE0_IDX           RHEA_DIE_ID2IDX(RHEA_DIE0_ID)
#if CONFIG_RHEA_DIE_MAX > 1
#define RHEA_DIE1_IDX           RHEA_DIE_ID2IDX(RHEA_DIE1_ID)
#if CONFIG_RHEA_DIE_MAX > 2
#define RHEA_DIE2_IDX           RHEA_DIE_ID2IDX(RHEA_DIE2_ID)
#define RHEA_DIE3_IDX           RHEA_DIE_ID2IDX(RHEA_DIE3_ID)
#endif /* CONFIG_RHEA_DIE_MAX > 2 */
#endif /* CONFIG_RHEA_DIE_MAX > 1 */

enum rhea_d2d_lock {
    D2D_LOCK_TILE_SEL = 0,
    D2D_LOCK1,
    D2D_LOCK2,
    D2D_LOCK3,
    D2D_LOCK4,
    D2D_LOCK5,
    D2D_LOCK6,
    D2D_LOCK7,
    D2D_LOCK8,
    D2D_LOCK9,
    D2D_LOCK10,
    D2D_LOCK11,
    D2D_LOCK12,
    D2D_LOCK13,
    D2D_LOCK14,
    D2D_LOCK15,

    D2D_LOCK_MAX,
};

typedef void (*d2d_irq_handler_t)(uint8_t int_idx, uint32_t cmd, uint32_t data);

void rhea_d2d_cfg_writel(uint8_t die, uint32_t val, uint32_t offset);
uint32_t rhea_d2d_cfg_readl(uint8_t die, uint32_t offset);
int rhea_d2d_write_data(const void *src, uint64_t addr, uint32_t count);
int rhea_d2d_read_data(void *dest, uint64_t addr, uint32_t count);
int rhea_d2d_writeb(uint8_t val, uint64_t addr);
int rhea_d2d_readb(uint8_t *val, uint64_t addr);
int rhea_d2d_writew(uint16_t val, uint64_t addr);
int rhea_d2d_readw(uint16_t *val, uint64_t addr);
int rhea_d2d_writel(uint32_t val, uint64_t addr);
int rhea_d2d_readl(uint32_t *val, uint64_t addr);
int rhea_d2d_writeq(uint64_t val, uint64_t addr);
int rhea_d2d_readq(uint64_t *val, uint64_t addr);
int rhea_d2d_get_lock(uint8_t die, enum rhea_d2d_lock idx);
void rhea_d2d_release_lock(uint8_t die, enum rhea_d2d_lock idx);
int rhea_d2d_select_tile(uint8_t die, uint8_t tile_id, uint32_t wait_ms);
int rhea_d2d_get_sel_tile(uint8_t die, uint8_t *tile_id);
void rhea_d2d_release_tile(void);
/**
 * Before enabling interrupt reception, make sure GIC_Init()
 * has been called to complete GIC initialization.
 */
void rhea_d2d_irq_recv_enable(d2d_irq_handler_t handler);
int rhea_d2d_send_int2ap(uint8_t die, uint32_t timeout_ms,
                        uint8_t int_idx, uint32_t cmd, uint32_t data);
int rhea_d2d_send_int2pcie(uint32_t timeout_ms, uint32_t cmd, 
                            uint8_t int_idx, uint32_t data);
void *rhea_d2d_get_dnoc_addr(void);
void *rhea_d2d_get_cnoc_addr(void);
int rhea_d2d_init(void);
void rhea_clci_clk_init(void);

#endif /* __D2D_API_H__ */