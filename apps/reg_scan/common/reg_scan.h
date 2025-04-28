#ifndef REG_SCAN_H
#define REG_SCAN_H

#include <stdio.h>
#include "chip.h"

#define RW_TEST(reg) reg = reg
#define RO_TEST(reg) (void)reg
#define WO_TEST(reg, val) reg = val

#endif // REG_SCAN_H__