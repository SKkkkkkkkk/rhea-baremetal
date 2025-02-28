#ifndef __LPI_H__
#define __LPI_H__

#include "memmap.h"

uint32_t init_gic(void);
int lpi_init(void);
void register_lpi(uint32_t device, uint32_t event, uint32_t hwirq);

#endif // __LPI_H__
