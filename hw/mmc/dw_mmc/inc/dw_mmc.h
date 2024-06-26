/*
 * Copyright (c) 2016-2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef DW_MMC_H
#define DW_MMC_H

#include "mmc.h"

#define DWMMC_DMA_MAX_BUFFER_SIZE (512 * 8)

int dw_mmc_init(void);

#endif /* DW_MMC_H */
