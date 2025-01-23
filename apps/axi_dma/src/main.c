#include <stdio.h>
#include "dw_axi_dma.h"
#include "systimer.h"

#define IS_PLD 0
#define SHOW_TEST_DATA 0

#if IS_PLD == 1
#define TCM_04_CFG_BASE            0x0015000000
#define TCM_14_CFG_BASE            0x8a22000000
#define TCM_15_CFG_BASE            0x8aa2000000
#define TCM_24_CFG_BASE            0x9222000000
#define TCM_25_CFG_BASE            0x92a2000000

#define TILE_1_4_DRAM_BASE 0x1440000000
#define TILE_2_4_DRAM_BASE 0x2440000000
#endif

#define DATA_SIZE 0x100

static uint64_t SRC_ADDR = 0x100000;
static uint64_t DST_ADDR = 0x100000 + DATA_SIZE * 4;

#if IS_PLD == 1
void mc_init(uint64_t addr, uint8_t layer) {
    // global
    if (layer == 4) {
        REG32(addr+0x00013054) = 0x00000000;
        REG32(addr+0x00013004) = 0x00001000; /* 2GB: 0x00001000, 512MB: 0x00000000 */
        REG32(addr+0x00013004) = 0x80001000; /* 2GB: 0x80001000, 512MB: 0x80000000 */
    } else {
        REG32(addr+0x00013054) = 0x00000000;
        REG32(addr+0x00013004) = 0x00000010;
        REG32(addr+0x00013004) = 0x80000010;
    }

    // bank
    uint32_t i, j, k;
    for (i = 0; i < 72; i++) {
        j = i / 18;
        k = i + j; // skip hub regs
        REG32(addr+k*0x400+0x004) = 0x00000005;
        REG32(addr+k*0x400+0x004) = 0x00000001;
        REG32(addr+k*0x400+0x004) = 0x80000001;
    }
}
#endif

int main (void)
{
    printf("enter main\n");

#ifdef TEST_SPEED
    uint64_t delta = 0;
    systimer_id_t timer;
#endif

    DMA_Channel_t ch;
    dma_config_t config;
    int i;

#ifdef TEST_SPEED
    uint64_t delta = 0;
    systimer_id_t timer;

    systimer_init();
#endif

#if IS_PLD == 1
    mc_init(TCM_04_CFG_BASE, 4);

    mc_init(TCM_14_CFG_BASE, 4);
    mc_init(TCM_15_CFG_BASE, 4);
    mc_init(TCM_24_CFG_BASE, 4);
    mc_init(TCM_25_CFG_BASE, 4);
#endif

    for(ch = DMA_CHANNEL_1; ch <= DMA_CHANNEL_8; ch++) {
        printf("mem-to-mem test, chan:%d start\n", ch);

        /* init source data */
        for(i = 0; i < DATA_SIZE; i++) {
            REG32(SRC_ADDR + i * 0x4) = 0x10000 * ch + i;
        }

        /* config dma chanel */
        config.ch = ch;
        config.dir = MEM_TO_MEM;
        config.sar = SRC_ADDR;
        config.dar = DST_ADDR;
        config.size = DATA_SIZE;
        config.is_src_addr_increse = SRC_ADDR_INCREMENT;
        config.is_dst_addr_increse = DST_ADDR_INCREMENT;
        config.handle_shake = 0;
        dma_config(&config);

#ifdef TEST_SPEED
        timer = systimer_acquire_timer();
#endif

        /* start transfer */
        dma_channel_start(ch);
        while (1) {
            if(is_dma_channel_transfer_done(ch)) {
                clear_channel_transfer_done_irq(ch);
                break;
            }
        }

#ifdef TEST_SPEED
    delta = systimer_get_elapsed_time(timer, IN_CNT);
    systimer_release_timer(timer);
    printf("mem-to-mem test, chan:%d,size:0x%x,time:%luns\n", ch, DATA_SIZE, delta);
#endif

        /* compare source data to dest data */
        for(i = 0; i < DATA_SIZE; i++) {
#if SHOW_TEST_DATA == 1
                printf("src addr:0x%lx, src data: 0x%x\n", SRC_ADDR + i * 0x4,
                                                         REG32(SRC_ADDR + i * 0x4));
                printf("dst addr:0x%lx, dst data: 0x%x\n", DST_ADDR + i * 0x4,
                                                         REG32(DST_ADDR + i * 0x4));
#endif
            if(REG32(SRC_ADDR + i * 0x4)!= REG32(DST_ADDR + i * 0x4)) {
                printf("mem-to-mem test, chan:%d,result:fail\n", ch);
            }
        }
        printf("mem-to-mem test, chan:%d,result:success\n", ch);
    }
}

