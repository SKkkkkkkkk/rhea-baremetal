#ifndef _DOOR_BELL_H_
#define _DOOR_BELL_H_

#include <stdint.h>
#include "config.h"
#include "interrupt.h"

void db_clear_ext_interrupt(uint64_t db_reg);
void db_init(uint64_t db_reg, irq_handler_t p_isr);
int db_is_using(uint64_t db_reg);
void db_generate_int(uint64_t db_reg, int32_t db_Num);

#endif
