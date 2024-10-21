#include <stdio.h>

#include "d2d_test.h"
#include "d2d_api.h"
#include "io.h"

static inline void delay(uint32_t value)
{
	volatile uint32_t i, j;

	for(i = 0; i < value; i++)
		for(j = 0; j < 1000; j++);
}

int run_die0_test(void)
{
	int ret;
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

int run_die1_test(void)
{
	int ret;
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

	while (1) {
		printf("[%d]%s 0x0040000000: 0x%x\n", __LINE__, __func__, 
				readl((void *) (uintptr_t) 0x0040000000));
		printf("[%d]%s 0x0040000010: 0x%x\n", __LINE__, __func__, 
				readl((void *) (uintptr_t) 0x0040000010));
		printf("[%d]%s 0x2640000000: 0x%x\n", __LINE__, __func__, 
				readl((void *) (uintptr_t) 0x2640000000));
		printf("[%d]%s 0x2640000010: 0x%x\n", __LINE__, __func__, 
				readl((void *) (uintptr_t) 0x2640000010));
		printf("[%d]%s 0x2740000000: 0x%x\n", __LINE__, __func__, 
				readl((void *) (uintptr_t) 0x2740000000));
		printf("[%d]%s 0x2740000010: 0x%x\n", __LINE__, __func__, 
				readl((void *) (uintptr_t) 0x2740000010));
		printf("[%d]%s 0x3640000000: 0x%x\n", __LINE__, __func__, 
				readl((void *) (uintptr_t) 0x3640000000));
		printf("[%d]%s 0x3640000010: 0x%x\n", __LINE__, __func__, 
				readl((void *) (uintptr_t) 0x3640000010));
		printf("[%d]%s 0x3740000000: 0x%x\n", __LINE__, __func__, 
				readl((void *) (uintptr_t) 0x3740000000));
		printf("[%d]%s 0x3740000010: 0x%x\n", __LINE__, __func__, 
				readl((void *) (uintptr_t) 0x3740000010));
	}
	return 0;
}
