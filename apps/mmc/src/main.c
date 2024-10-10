#include <stdio.h>
#include <string.h>
#include <errno.h>

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

static systimer_id_t timer;

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
            printf("[%p] ", mem + i);
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

#define TEST_APRAM_ADDR     (0x120000)
#define TEST_DDR_ADDR       (0x40000000)
#define TEST_VERIFY_ADDR    (0x41000000)
#define TEST_START_BLK      (1024)
#define TEST_DATA_CNT       (512)  // 1024 * 512 = 512KB
#define TEST_DATA_LEN       (MMC_BLOCK_SIZE * TEST_DATA_CNT)

#define TEST_ARR(buf, i)  *((unsigned char *) ((buf) + (i)))

int read_test(uintptr_t addr, int blk, size_t size)
{
    int ret;
    size_t i;
    uint64_t start = 0;
    uint64_t elapsed = 0;
    uint64_t tmp_addr;

    memset((void *) addr, 0, size);
    start = systimer_get_elapsed_time(timer, IN_US);
    ret = mmc_read_blocks(blk, addr, size);
    elapsed = systimer_get_elapsed_time(timer, IN_US) - start;
    if (ret != TEST_DATA_LEN) {
        printf("Read size does not match expectations\n");
        return -EIO;
    }
    for (i = 0; i < size; i++) {
        if (TEST_ARR(addr, i) != (i & 0xff)) {
            printf("Verification failed, got 0x%x at index %ld(0x%lx), expected 0x%lx\n",
                    TEST_ARR(addr, i), i, addr + i, i & 0xff);
            tmp_addr = (addr + i - 0xf) & ~0xf;
            if (tmp_addr < addr) tmp_addr = addr;
            dump_mem((void *) tmp_addr, 128);
            break;
            // return -EIO;
        }
    }
    return calculate_KBs(elapsed, size);
}

int write_test(uintptr_t addr, int blk, size_t size)
{
    int ret;
    size_t i;
    uint64_t start = 0;
    uint64_t elapsed = 0;

    for (i = 0; i < TEST_DATA_LEN; i++)
        TEST_ARR(addr, i) = i & 0xff;
    start = systimer_get_elapsed_time(timer, IN_US);
    ret = mmc_write_blocks(blk, addr, size);
    elapsed = systimer_get_elapsed_time(timer, IN_US) - start;
    if (ret != (TEST_DATA_LEN)) {
        printf("Write size does not match expectations\n");
        return -EIO;
    }
    return calculate_KBs(elapsed, size);
}

int boot_operation_test(void)
{
    int ret;
    uint64_t start = 0;
    uint64_t elapsed = 0;

    printf("%s Start boot operation test ...\n", __func__);

    start = systimer_get_elapsed_time(timer, IN_US);
    ret = dw_boot_start((uintptr_t) TEST_DDR_ADDR, TEST_DATA_LEN);
    elapsed = systimer_get_elapsed_time(timer, IN_US) - start;
    if (ret) {
        printf("Boot operation failed\n");
        return ret;
    }
    printf("It takes %ldus to read 0x%x bytes to memory (%ld KB/s)\n", 
            elapsed, TEST_DATA_LEN, 
            calculate_KBs(elapsed, TEST_DATA_LEN));
    // dump_mem((void *) TEST_DDR_ADDR, TEST_DATA_LEN);
    printf("%s Test done\n\n", __func__);
    return 0;
}

int boot_partition_test(void)
{
    int ret, i;
    uint64_t start = 0;
    uint64_t elapsed = 0;

    printf("%s Start boot partition write protection test ...\n", __func__);
    printf("Start preparing test data ...\n");

	ret = mmc_part_switch_current_boot();
	if (ret < 0) return ret;

    memset((void *) TEST_DDR_ADDR, 0, TEST_DATA_LEN);
    start = systimer_get_elapsed_time(timer, IN_US);
    ret = mmc_write_blocks(TEST_START_BLK, TEST_DDR_ADDR, TEST_DATA_LEN);
    if (ret != TEST_DATA_LEN) {
        printf("Write size does not match expectations\n");
        return ret;
    }
    elapsed = systimer_get_elapsed_time(timer, IN_US) - start;
    printf("It takes %ldus to write 0x%x bytes of memory data to block %d (%ld KB/s)\n", 
            elapsed, TEST_DATA_LEN, TEST_START_BLK, 
            calculate_KBs(elapsed, TEST_DATA_LEN));
    ret = mmc_read_blocks(TEST_START_BLK, TEST_DDR_ADDR, MMC_BLOCK_SIZE);
    if (ret != MMC_BLOCK_SIZE) {
        printf("Read size does not match expectations\n");
        return ret;
    }
    for (i = 0; i < MMC_BLOCK_SIZE; i++) {
        if (TEST_ARR((uintptr_t) TEST_DDR_ADDR, i) != 0) {
            if (mmc_get_wp_status() == 0x01)
                printf("Boot area 1 is power on protected, skip test ...\n");
            else
                printf("Failed to prepare test data in boot partition\n");

            return ret;
        }
        ret = i;
    }

    for (i = 0; i < TEST_DATA_LEN; i++)
        TEST_ARR((uintptr_t) TEST_DDR_ADDR, i) = i & 0xff;
    ret = mmc_write_blocks(TEST_START_BLK, TEST_DDR_ADDR, TEST_DATA_LEN >> 1);
    if (ret != (TEST_DATA_LEN >> 1)) {
        printf("Write size does not match expectations\n");
        return ret;
    }
    mmc_boot_pwr_wp(MMC_BOOT_AREA1);
    ret = mmc_write_blocks(TEST_START_BLK + (TEST_DATA_LEN >> 10), /* TEST_DATA_LEN / 2 / 512 */
                     TEST_DDR_ADDR + (TEST_DATA_LEN >> 1),
                     TEST_DATA_LEN >> 1);
    if (ret != (TEST_DATA_LEN >> 1)) {
        printf("Write size does not match expectations\n");
        return ret;
    }

	ret = mmc_part_switch_user();
	if (ret < 0)
        return ret;

    printf("Start reading test data ...\n");
    memset((void *) TEST_DDR_ADDR, 0, TEST_DATA_LEN);
    start = systimer_get_elapsed_time(timer, IN_US);
    ret = mmc_boot_part_read_blocks(TEST_START_BLK, TEST_DDR_ADDR, TEST_DATA_LEN);
    if (ret != TEST_DATA_LEN) {
        printf("Read size does not match expectations\n");
        return ret;
    }
    elapsed = systimer_get_elapsed_time(timer, IN_US) - start;
    printf("It takes %ldus to read 0x%x bytes of mmc block %d to memory (%ld KB/s)\n", 
            elapsed, TEST_DATA_LEN, TEST_START_BLK, 
            calculate_KBs(elapsed, TEST_DATA_LEN));
    // dump_mem((void *) TEST_DDR_ADDR, TEST_DATA_LEN);
    printf("Start verifying data ...\n");
    ret = 0;
    for (i = 0; i < TEST_DATA_LEN; i++) {
        if (i < (TEST_DATA_LEN >> 1)) {
            if (TEST_ARR((uintptr_t) TEST_DDR_ADDR, i) != (i & 0xff)) {
                printf("Verification failed, got %d at index %d, expected %d\n",
                        TEST_ARR((uintptr_t) TEST_DDR_ADDR, i), i, i & 0xff);
                return -EFAULT;
            }
        } else {
            if (TEST_ARR((uintptr_t) TEST_DDR_ADDR, i) != 0) {
                printf("Write protection failure, got %d at index %d\n",
                        TEST_ARR((uintptr_t) TEST_DDR_ADDR, i), i);
                return -EFAULT;
            }
        }
    }
    printf("%s Verification successful\n\n", __func__);
    return 0;
}

int burst_size_test(void)
{
    /**
     * Note: Due to alignment testing requirements, the following 
     *  assertions need to be commented out.
     *  dw_mmc.c dw_prepare()        assert((buf & DWMMC_ADDRESS_MASK) == 0)
     *  mmc.c    mmc_write_blocks()  assert((buf & MMC_BLOCK_MASK) == 0U)
     */
    const unsigned int align_offset[] = {4, 8, 32, 64, 128, 512, 4096};
    unsigned char burst_size, offset, is_apram;
    unsigned int addr_base;
    int ret;

    printf("%s Start burst size test ...\n\n", __func__);

    for (burst_size = 0; burst_size <= 0x3; burst_size++) {
        dw_set_dma_burst_size(burst_size);
        for (offset = 0; offset < sizeof(align_offset) / sizeof(align_offset[0]); offset++) {
            for (is_apram = 0; is_apram < 2; is_apram++) {
                addr_base = is_apram ? TEST_APRAM_ADDR : TEST_DDR_ADDR;
                addr_base += align_offset[offset];

                ret = write_test(addr_base, TEST_START_BLK, TEST_DATA_LEN);
                if (ret < 0) return ret;
                printf("burst_size: 0x%x address: 0x%08x(+%d) write speed: %d KB/s\n",
                        burst_size, addr_base, align_offset[offset], ret);
                memset((void *) TEST_VERIFY_ADDR, 0, TEST_DATA_LEN);
                ret = read_test(TEST_VERIFY_ADDR, 
                                TEST_START_BLK, TEST_DATA_LEN);
                if (ret < 0) return ret;
                printf("\n");

                memset((void *) TEST_VERIFY_ADDR, 0, TEST_DATA_LEN);
                ret = write_test(TEST_VERIFY_ADDR,
                                TEST_START_BLK, TEST_DATA_LEN);
                if (ret < 0) return ret;
                ret = read_test(addr_base, TEST_START_BLK, TEST_DATA_LEN);
                if (ret < 0) return ret;
                printf("burst_size: 0x%x address: 0x%08x(+%d) read speed: %d KB/s\n\n",
                        burst_size, addr_base, align_offset[offset], ret);
            }
        }
    }
    printf("%s Test done\n\n", __func__);
    return 0;
}

int main()
{
    int ret = 0;

    systimer_init();
    timer = systimer_acquire_timer();

    ret = boot_operation_test();
    if (ret) goto err;

    ret = dw_mmc_init();
    if (ret) {
        printf("Initialization failed\n");
        goto err;
    }

    ret = boot_partition_test();
    if (ret) goto err;

    ret = burst_size_test();
    if (ret) goto err;

err:
    systimer_release_timer(timer);
    if (ret)
        printf("Obtain a return value of %d\n", ret);
    while (1);
    return 0;
}