#ifndef __D2D_API_H__
#define __D2D_API_H__

#include <stdint.h>

#define rhea_tile_id_hex(row, col)  (((row & 0xf) << 4) | (col & 0xf))
#define rhea_tile_id_col(hex)       (hex & 0xf)
#define rhea_tile_id_row(hex)       ((hex >> 4) & 0xf)

enum rhea_die {
    RHEA_DIE0 = 0,
#if defined(CONFIG_RHEA_DUAL_DIE) || defined(CONFIG_RHEA_QUAD_DIE)
    RHEA_DIE1,
#if defined(CONFIG_RHEA_QUAD_DIE)
    RHEA_DIE2,
    RHEA_DIE3,
#endif /* defined(CONFIG_RHEA_DUAL_DIE) || defined(CONFIG_RHEA_QUAD_DIE) */
#endif /* defined(CONFIG_RHEA_QUAD_DIE) */

    RHEA_DIE_MAX,
};

enum rhea_d2d_lock {
    D2D_LOCK_TILE_SEL = 0,
    D2D_LOCK_CMD,
    D2D_LOCK_CMD_DATA,
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

static inline void writeb(uint8_t value, void *address)
{
	uintptr_t addr = (uintptr_t)address;

	*((volatile uint8_t *)(addr)) = value;
}

static inline uint8_t readb(void *address)
{
	uintptr_t addr = (uintptr_t)address;

	return *((volatile uint8_t *)(addr));
}

static inline void writew(uint16_t value, void *address)
{
	uintptr_t addr = (uintptr_t)address;

	*((volatile uint16_t *)(addr)) = value;
}

static inline uint16_t readw(void *address)
{
	uintptr_t addr = (uintptr_t)address;

	return *((volatile uint16_t *)(addr));
}

static inline void writel(uint32_t value, void *address)
{
	uintptr_t addr = (uintptr_t)address;

	*((volatile uint32_t *)(addr)) = value;
}

static inline uint32_t readl(void *address)
{
	uintptr_t addr = (uintptr_t)address;

	return *((volatile uint32_t *)(addr));
}

int rhea_d2d_write_data(const void *src, uint64_t addr, uint32_t count);
int rhea_d2d_read_data(void *dest, uint64_t addr, uint32_t count);
int rhea_d2d_writeb(uint8_t val, uint64_t addr);
int rhea_d2d_readb(uint8_t *val, uint64_t addr);
int rhea_d2d_writew(uint16_t val, uint64_t addr);
int rhea_d2d_readw(uint16_t *val, uint64_t addr);
int rhea_d2d_writel(uint32_t val, uint64_t addr);
int rhea_d2d_readl(uint32_t *val, uint64_t addr);
int rhea_d2d_get_lock(enum rhea_die die, enum rhea_d2d_lock idx);
void rhea_d2d_release_lock(enum rhea_die die, enum rhea_d2d_lock idx);
int rhea_d2d_select_tile(enum rhea_die die, uint8_t tile_id, uint32_t wait_ms);
int rhea_d2d_get_sel_tile(enum rhea_die die, uint8_t *tile_id);
void rhea_d2d_release_tile(void);
void *rhea_d2d_get_ioaddr(void);
int rhea_d2d_init(void);

#endif /* __D2D_API_H__ */