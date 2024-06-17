#ifndef __DELAY_H__
#define __DELAY_H__

#ifdef __cplusplus
 extern "C" {
#endif
#include <stdint.h>
#include <arch_features.h>

static inline void udelay(unsigned int usec)
{
	uint64_t start_count_val = syscounter_read();
	uint64_t wait_cycles = (usec * read_cntfrq_el0()) / 1000000;

	while ((syscounter_read() - start_count_val) < wait_cycles)
		/* Busy wait... */;
}

static inline void mdelay(unsigned int msec)
{
	udelay(msec * 1000);
}

static inline void delay(unsigned int sec)
{
	mdelay(sec * 1000);
}

#ifdef __cplusplus
}
#endif

#endif