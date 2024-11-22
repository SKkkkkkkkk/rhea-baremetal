#include <stdio.h>
#include "dma.h"
#include "systimer.h"

#define reg32(addr) (*(volatile uint32_t *)(uintptr_t)(addr))

#define TCM_04_CFG_BASE            0x0015000000
#define TCM_14_CFG_BASE            0x8a22000000
#define TCM_15_CFG_BASE            0x8aa2000000
#define TCM_24_CFG_BASE            0x9222000000
#define TCM_25_CFG_BASE            0x92a2000000

#define DATA_SIZE 0x100
#define TILE_1_4_DRAM_BASE 0x1440000000
#define TILE_2_4_DRAM_BASE 0x2440000000

static uint64_t SRC_ADDR = AP_DRAM_BASE;
static uint64_t DST_ADDR = AP_DRAM_BASE + DATA_SIZE;

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

int main (void)
{
    printf("enter main\n");

    mc_init(TCM_04_CFG_BASE, 4);

    mc_init(TCM_14_CFG_BASE, 4);
    mc_init(TCM_15_CFG_BASE, 4);
    mc_init(TCM_24_CFG_BASE, 4);
    mc_init(TCM_25_CFG_BASE, 4);

    for(int i = 0; i < DATA_SIZE; i++) {
        REG32(SRC_ADDR + i * 0x4) = 0x100 + i;
    }

#ifdef TEST_SPEED
    uint64_t delta = 0;
    systimer_id_t timer;
#endif

    unsigned low_src = (uint32_t)(SRC_ADDR);
    unsigned high_src = (uint32_t)(SRC_ADDR >> 32);
    unsigned low_dst = (uint32_t)(DST_ADDR);
    unsigned high_dst = (uint32_t)(DST_ADDR >> 32);

    DMA -> CH1_SAR_0 = low_src; //source address low 32 bits
    DMA -> CH1_SAR_32 = high_src; //source address high 32 bits
    DMA -> CH1_DAR_0 = low_dst; //destination address low 32 bits
    DMA -> CH1_DAR_32 = high_dst; //destination address high 32 bits

    DMA -> CH1_CTL_0 = 
        AXI_MASTER_0                         << 0| //source select master 1
        AXI_MASTER_0                         << 2| //destination select master 1
        SRC_ADDR_INCREMENT                   << 4| //source address growth
        DST_ADDR_INCREMENT                   << 6| //destination address growth 
        SRC_TRANSFER_WIDTH_64                << 8| //sourec transfer width is 64 bits
        DST_TRANSFER_WIDTH_64                <<11| //destination transfer width is 64 bits
        //SRC_MSIZE_1                        <<14| //memory not need transaction
        //DST_MSIZE_1                        <<18| //memory not need transaction
        0                                    <<30;

    DMA -> CH1_CTL_32 = 
        ARLEN_EN                               <<(38-32)| //source burst length enable
        7                                      <<(39-32)| //source burst length is 0x4
        AWLEN_EN                               <<(47-32)| //destination burst length enable
        7                                      <<(48-32)| //destination burst length is 0x4
        SRC_STATUS_DISABLE                     <<(56-32)| //disable fetch status from source peripheral of channelx pointed to by the content of CHx_SSTATAR
        DST_STATUS_DISABLE                     <<(57-32)| //disable fetch status from destination peripheral of channelx pointed to by the content of CHx_DSTATAR
        INTDISABLE_COMPLETOFBLKTRANS_SHADORLLI <<(58-32)| //disables CHx_IntStatusReg.BLOCK_TFR_DONE_IntStat field
        NOTLAST_SHADORLLI                      <<(62-32)| //indicates shadow register content or the linked list item fetched from the memory not's the last one
        SHADORLLI_INVALID                      <<(63-32); //indicates the content of shadow register or the linked list item fetched from the memory is invalid

    DMA -> CH1_BLOCK_TS_0 = (DATA_SIZE >> SRC_TRANSFER_WIDTH_64) - 1; // b:32 BLOCK_TS //write data number

    DMA -> CH1_CFG2_0 = 
        SRC_CONTIGUOUS        << 0| //source block transfer type is single block
        DST_CONTIGUOUS        << 2; //destination block transfer type is single block

    DMA -> CH1_CFG2_32 = 
        MEM_TO_MEM_DMAC        <<(32-32)| //transfer type and Flow Control model are respectively Memory to Memory,DW_axi_dmac
        CHANNEL_PRIORITY7      <<(47-32)| //channel priority is 7
        CHANNEL_LOCK_DISABLE   <<(52-32)| //This channel not locks the host interface when other channels want to get host approval
        8                      <<(55-32)| //Source Outstanding Request Limit == 3
        8                      <<(59-32); //Destination Outstanding Request Limit == 3

    DMA -> CH1_INTSTATUS_ENABLEREG_0 = 0xffffffff; //Enable interrupt generation bit is valid
    DMA -> CH1_INTSIGNAL_ENABLEREG_0 = 0xffffffff; //Enable interrupt generation bit is valid

#ifdef TEST_SPEED
    systimer_init();
    timer = systimer_acquire_timer();
#endif

    DMA -> DMAC_CFGREG_0  = 0x3; //enable DMAC and its interrupt logic
    DMA -> DMAC_CHENREG_0 = 0x101; //EN channel0  while(1)

    // while generate DMA transfer completed interface
    while (1) {
        if(((DMA -> CH1_INTSTATUS_0) & 0x00000002) == 0x00000002) 
            break;
    }

#ifdef TEST_SPEED
    delta = systimer_get_elapsed_time(timer, IN_CNT);
    systimer_release_timer(timer);
    printf("dma MEM2MEM: size:0x%x time:%luns\n\r",DATA_SIZE, delta);
#endif

    if(REG32(SRC_ADDR) != REG32(DST_ADDR))
        printf("failed\r\n");
    else
        printf("pass\r\n");

    printf("read src val: 0x%x\r\n",REG32(SRC_ADDR));
    printf("read dst val: 0x%x\r\n",REG32(DST_ADDR));

    return 0;
}

