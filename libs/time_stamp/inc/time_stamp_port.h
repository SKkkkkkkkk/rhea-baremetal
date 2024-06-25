#ifndef SHARED_RAM_BASE
#define SHARED_RAM_BASE 0x00100000
#endif
#define TIME_STAMP_BASE (SHARED_RAM_BASE + 40) // sec_entrypoint(8B) + PLAT_RHEA_HOLD_STATE(8B) * 4cores
#define TIME_STAMP_FREQ 24000000
#define TIME_STAMP_CNT_BITS 64
static inline uint64_t ts_read_cntpct_el0(void) 
{ uint64_t v; __asm__ volatile ("mrs %0, " "cntpct_el0" : "=r" (v)); return v; }
#define TIME_STAMP_GET_TICK() ts_read_cntpct_el0()