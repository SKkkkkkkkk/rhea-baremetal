#pragma once

#include <stdint.h>
#include "memmap.h"
#include "core_v5.h"
#ifndef VIRT
#   define NDS_PLIC_BASE        PLIC_BASE
#   define NDS_PLIC_SW_BASE     PLIC_SW_BASE
#else
#   define NDS_PLIC_BASE        VIRT_PLIC
// #   define NDS_PLIC_SW_BASE     PLIC_SW_BASE
#endif
#include "plic.h"

typedef void (*irq_handler_t) (void);
void irq_set_handler(uint16_t int_id, irq_handler_t handler);
irq_handler_t irq_get_handler(uint16_t int_id);