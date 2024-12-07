#include <config.h>
// #include <platform.h>
#include <stdio.h>
#include <mmio.h>
// #include <interrupt.h>
#include <doorbell.h>
#include <arch.h>

// void db_clear_ext_interrupt(uint32_t db_reg)
// {
// 	mmio_write_32(db_reg, 0);

// 	/* Sync the output. Make sure not to progress until the write to external register is done */
// 	irq_clear_pending(DB_INTERRUPT_ID);
// 	inst_fencei();
// }

// void db_init(uint32_t db_reg, irq_handler_t p_isr)
// {
// 	db_clear_ext_interrupt(db_reg);

// 	request_irq(DB_INTERRUPT_ID, p_isr, IRQ_FLAGS_COMMON_HIGH, "doorbell", NULL);
// }

int db_is_using(uint64_t db_reg)
{
	return !!mmio_read_32(db_reg);
}

void db_generate_int(uint64_t db_reg, int32_t db_Num)
{
	mmio_write_32(db_reg, db_Num);
}
