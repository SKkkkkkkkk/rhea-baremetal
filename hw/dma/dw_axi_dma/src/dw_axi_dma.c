#include "dw_axi_dma.h"

uint8_t dma_config(dma_config_t* config)
{
    DMA_Channel_t ch = config->ch;
    uint8_t hardware_hs = SRC_SOFTWARE_HS;

    if((ch < DMA_CHANNEL_1) || (ch > DMA_CHANNEL_8))
        return 1;

    REG32(DMA_BASE + CH1_SAR_0 + ((ch-1) * 0x100)) = (uint32_t)config->sar;
    REG32(DMA_BASE + CH1_SAR_32 + ((ch-1) * 0x100)) = (uint32_t)(config->sar >> 32);
    REG32(DMA_BASE + CH1_DAR_0 + ((ch-1) * 0x100)) = (uint32_t)config->dar;
    REG32(DMA_BASE + CH1_DAR_32 + ((ch-1) * 0x100)) = (uint32_t)(config->dar >> 32);

    REG32(DMA_BASE + CH1_CTL_0 + ((ch-1) * 0x100)) =
                                            AXI_MASTER_0                << 0|
                                            AXI_MASTER_0                << 2|
                                            config->is_src_addr_increse << 4|
                                            config->is_dst_addr_increse << 6|
                                            SRC_TRANSFER_WIDTH_32       << 8|
                                            SRC_TRANSFER_WIDTH_32       <<11|
                                            NONPOSTED_LASTWRITE_EN      <<30;

    REG32(DMA_BASE + CH1_CTL_32 + ((ch-1) * 0x100)) =
                                            ARLEN_EN                               <<(38 - 32)|
                                            7                                      <<(39 - 32)|
                                            AWLEN_EN                               <<(47 - 32)|
                                            7                                      <<(48 - 32)|
                                            SRC_STATUS_DISABLE                     <<(56 - 32)|
                                            DST_STATUS_DISABLE                     <<(57 - 32)|
                                            INTDISABLE_COMPLETOFBLKTRANS_SHADORLLI <<(58 - 32)|
                                            NOTLAST_SHADORLLI                      <<(62 - 32)|
                                            SHADORLLI_INVALID                      <<(63 - 32);

    REG32(DMA_BASE + CH1_BLOCK_TS_0 + ((ch-1) * 0x100)) = config->size - 1;

    REG32(DMA_BASE + CH1_CFG2_0 + ((ch-1) * 0x100)) = 
                                            SRC_CONTIGUOUS          << 0|
                                            DST_CONTIGUOUS          << 2|
                                            config->handle_shake    << 4;

    if( config->dir != MEM_TO_MEM)
        hardware_hs = SRC_HARDWARE_HS;

    REG32(DMA_BASE + CH1_CFG2_32 + ((ch-1) * 0x100)) =
                                            config->dir             <<(32-32)|
                                            hardware_hs             <<(35-32)|
                                            CHANNEL_PRIORITY7       <<(47-32)|
                                            CHANNEL_LOCK_DISABLE    <<(52-32)|
                                            7                       <<(55-32)|
                                            7                       <<(59-32);
    return 0;
}

void dma_channel_start(DMA_Channel_t ch)
{
    assert((ch <= DMA_CHANNEL_8) && (ch != NO_FREE_DMA_CHANNEL));
    REG32(DMA_BASE + (CH1_INTSTATUS_ENABLEREG_0 + ((ch - 1) * 0x100))) = 2;
    REG32(DMA_BASE + (CH1_INTSIGNAL_ENABLEREG_0 + ((ch - 1) * 0x100))) = 2;
    REG32(DMA_BASE + DMAC_CFGREG_0) = 0x3;
    REG32(DMA_BASE + DMAC_CHENREG_0) = 0x101 << (ch - 1);
}

bool is_dma_channel_transfer_done(DMA_Channel_t ch)
{
    assert((ch <= DMA_CHANNEL_8) && (ch != NO_FREE_DMA_CHANNEL));
    return (REG32(DMA_BASE + CH1_INTSTATUS_0 + (ch - 1) * 0x100) & 0x00000002);
}

void clear_channel_transfer_done_irq(DMA_Channel_t ch)
{
    assert((ch <= DMA_CHANNEL_8) && (ch != NO_FREE_DMA_CHANNEL));
    REG32(DMA_BASE + CH1_INTCLEARREG_0 + ((ch - 1) * 0x100)) = 0x00000002;
}