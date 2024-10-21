#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "d2d_test.h"
#include "d2d_api.h"
#include "io.h"

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
    return 0;
}
