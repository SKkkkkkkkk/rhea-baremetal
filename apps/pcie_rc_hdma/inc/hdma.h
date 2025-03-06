#ifndef __HDMA_H__
#define __HDMA_H__

#include "memmap.h"
#include "utils_def.h"

#define PLD_Z1      1
#define PLD_Z2      0

#define WR_CH0      1
#define RD_CH0      0
#define WR_CH1      0
#define RD_CH1      0

#define SIZE    0x200
#define SAR_ADDR 0x00400100000
#define DAR_ADDR 0x00400100000

#define SAR_HIGH_MASK ((SAR_ADDR) >> 32)
#define SAR_LOW_MASK ((SAR_ADDR) & 0xFFFFFFFF)
#define DAR_HIGH_MASK ((DAR_ADDR) >> 32)
#define DAR_LOW_MASK ((DAR_ADDR) & 0xFFFFFFFF)

#define HDMA_INT0   175
#define HDMA_INT1   176
#define HDMA_INT2   177
#define HDMA_INT3   178

void hdma_core_off(void);
void hdma_core_ch_status(void);
void dw_hdma_v0_core_handle_int(void);
void dw_hdma_v0_core_start(void);
void dw_hdma_v0_core_ch_config(void);
void hdma_write_data(uint64_t start_addr,uint64_t end_addr);

#endif // __HDMA_H__
