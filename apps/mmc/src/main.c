#include <stdio.h>
#include <string.h>

#include "dw_mmc.h"
#include "systimer.h"

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

#define TEST_START_BLK  (1024)
#define TEST_DATA_BUF   (0x120000)
#define TEST_DATA_CNT   (256)
#define TEST_DATA_LEN   (2 * MMC_BLOCK_SIZE * TEST_DATA_CNT)

#define TEST_ARR(i)  *((unsigned char *) ((TEST_DATA_BUF) + (i)))

int main()
{
    int ret = 0;
    unsigned long i;
    uint64_t start = 0;
    uint64_t elapsed = 0;
    systimer_id_t timer;

    printf("Start boot operation test ...\n");

    systimer_init();
    timer = systimer_acquire_timer();
    start = systimer_get_elapsed_time(timer, IN_US);

    ret = dw_boot_start((uintptr_t) TEST_DATA_BUF, TEST_DATA_LEN);
    if (ret) {
        printf("Boot operation failed\n");
        goto err;
    }
    elapsed = systimer_get_elapsed_time(timer, IN_US) - start;
    printf("It takes %ldus to read 0x%x bytes to memory (%ld KB/s)\n", 
            elapsed, TEST_DATA_LEN, 
            calculate_KBs(elapsed, TEST_DATA_LEN));
    // dump_mem((void *) TEST_DATA_BUF, TEST_DATA_LEN);
    printf("\n");

    printf("Start boot partition write protection test ...\n");

    ret = dw_mmc_init();
    if (ret) {
        printf("Initialization failed\n");
        goto err;
    }

    printf("Start preparing test data ...\n");

	ret = mmc_part_switch_current_boot();
	if (ret < 0)
        goto err;

    memset((void *) TEST_DATA_BUF, 0, TEST_DATA_LEN);
    start = systimer_get_elapsed_time(timer, IN_US);
    ret = mmc_write_blocks(TEST_START_BLK, TEST_DATA_BUF, TEST_DATA_LEN);
    if (ret != TEST_DATA_LEN) {
        printf("Write size does not match expectations\n");
        goto err;
    }
    elapsed = systimer_get_elapsed_time(timer, IN_US) - start;
    printf("It takes %ldus to write 0x%x bytes of memory data to block %d (%ld KB/s)\n", 
            elapsed, TEST_DATA_LEN, TEST_START_BLK, 
            calculate_KBs(elapsed, TEST_DATA_LEN));
    ret = mmc_read_blocks(TEST_START_BLK, TEST_DATA_BUF, MMC_BLOCK_SIZE);
    if (ret != MMC_BLOCK_SIZE) {
        printf("Read size does not match expectations\n");
        goto err;
    }
    for (i = 0; i < MMC_BLOCK_SIZE; i++) {
        if (TEST_ARR(i) != 0) {
            if (mmc_get_wp_status() == 0x01)
                printf("Boot area 1 is power on protected, skip test ...\n");
            else
                printf("Failed to prepare test data in boot partition\n");

            goto err;
        }
        ret = i;
    }

    for (i = 0; i < TEST_DATA_LEN; i++)
        TEST_ARR(i) = i & 0xff;
    ret = mmc_write_blocks(TEST_START_BLK, TEST_DATA_BUF, TEST_DATA_LEN >> 1);
    if (ret != (TEST_DATA_LEN >> 1)) {
        printf("Write size does not match expectations\n");
        goto err;
    }
    mmc_boot_pwr_wp(MMC_BOOT_AREA1);
    ret = mmc_write_blocks(TEST_START_BLK + (TEST_DATA_LEN >> 10), /* TEST_DATA_LEN / 2 / 512 */
                     TEST_DATA_BUF + (TEST_DATA_LEN >> 1),
                     TEST_DATA_LEN >> 1);
    if (ret != (TEST_DATA_LEN >> 1)) {
        printf("Write size does not match expectations\n");
        goto err;
    }

	ret = mmc_part_switch_user();
	if (ret < 0)
        goto err;

    printf("Start reading test data ...\n");
    memset((void *) TEST_DATA_BUF, 0, TEST_DATA_LEN);
    start = systimer_get_elapsed_time(timer, IN_US);
    ret = mmc_boot_part_read_blocks(TEST_START_BLK, TEST_DATA_BUF, TEST_DATA_LEN);
    if (ret != TEST_DATA_LEN) {
        printf("Read size does not match expectations\n");
        goto err;
    }
    elapsed = systimer_get_elapsed_time(timer, IN_US) - start;
    printf("It takes %ldus to read 0x%x bytes of mmc block %d to memory (%ld KB/s)\n", 
            elapsed, TEST_DATA_LEN, TEST_START_BLK, 
            calculate_KBs(elapsed, TEST_DATA_LEN));
    // dump_mem((void *) TEST_DATA_BUF, TEST_DATA_LEN);

    printf("Start verifying data ...\n");
    ret = 0;
    for (i = 0; i < TEST_DATA_LEN; i++) {
        if (i < (TEST_DATA_LEN >> 1)) {
            if (TEST_ARR(i) != (i & 0xff)) {
                printf("Verification failed, got %d at index %ld, expected %ld\n",
                        TEST_ARR(i), i, i & 0xff);
                goto err;
            }
        } else {
            if (TEST_ARR(i) != 0) {
                printf("Write protection failure, got %d at index %ld\n",
                        TEST_ARR(i), i);
                goto err;
            }
        }
    }
    printf("Verification successful\n");
    printf("\n");

err:
    systimer_release_timer(timer);
    if (ret)
        printf("Obtain a return value of %d\n", ret);
    while (1);
    return 0;
}