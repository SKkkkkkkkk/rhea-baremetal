#ifndef __LPI_H__
#define __LPI_H__

#include "memmap.h"

uint32_t init_gic(void);
int lpi_init(void);

#endif // __LPI_H__
