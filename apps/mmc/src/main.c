#include <stdio.h>
#include <string.h>

#include "dw_mmc.h"

#define TEST_DATA_CNT   (16)
#define TEST_DATA_LEN   (MMC_BLOCK_SIZE * TEST_DATA_CNT)

void dump_mem(const void *mem, size_t length) {
    const unsigned char *byte = (const unsigned char *)mem;
    for (size_t i = 0; i < length; i++) {
        printf("%02x ", byte[i]);
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
    if (length % 16 != 0) {
        printf("\n");
    }
}

int main()
{
    int ret = 0;
    int test_cnt = 0;
    int test_len = 0;
    unsigned char w_data[TEST_DATA_LEN] __aligned(512) = {0};
    unsigned char r_data[TEST_DATA_LEN] __aligned(512) = {0};

    printf("Starting test for mmc ...\n");

    ret = dw_mmc_init();
    if (ret) {
        printf("init failed, ret = %d\n", ret);
        while (1);
    }

    while (1)
    {
        if (++test_cnt > TEST_DATA_CNT)
            while (1);

        test_len = MMC_BLOCK_SIZE * test_cnt;
        memset(w_data, test_cnt, test_len);
        mmc_write_blocks(test_cnt, (uintptr_t) w_data, test_len);
        memset(r_data, 0x0, sizeof(r_data));
        mmc_read_blocks(test_cnt, (uintptr_t) r_data, test_len);
        // dump_mem(r_data, sizeof(r_data));
        ret = memcmp(w_data, r_data, test_len);
        printf("block %d, size %d -> %s\n", 
                test_cnt, test_len, ret ? "failed" : "pass");
        while (ret);
    }
    return 0;
}