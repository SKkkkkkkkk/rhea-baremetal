#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

#include <stdint.h>
#include <utils_def.h>

#define IRQ_FLAGS_TYPE_LEVEL					(0 << 0)
#define IRQ_FLAGS_TYPE_EDGE					(1 << 0)
#define IRQ_FLAGS_TYPE_MASK					GENMASK(0, 0)

#define IRQ_FLAGS_SET_PRIORITY					(1 << 1)
#define IRQ_FLAGS_PRIORITY(v)					((v << 2) & IRQ_FLAGS_PRIORITY_MASK)
#define IRQ_FLAGS_PRIORITY_MASK					GENMASK(5, 2)

#define IRQ_FLAGS_POLARITY_HIGH					(0 << 6)
#define IRQ_FLAGS_POLARITY_LOW					(1 << 6)
#define IRQ_FLAGS_POLARITY_MASK					GENMASK(6, 6)

#define IRQ_FLAGS_GET_TYPE(v)					FIELD_GET(IRQ_FLAGS_TYPE_MASK, v)
#define IRQ_FLAGS_GET_PRIORITY(v)				FIELD_GET(IRQ_FLAGS_PRIORITY_MASK, v)
#define IRQ_FLAGS_GET_POLARITY(v)				FIELD_GET(IRQ_FLAGS_POLARITY_MASK, v)

#define IRQ_FLAGS_COMMON_HIGH					(IRQ_FLAGS_TYPE_LEVEL | IRQ_FLAGS_POLARITY_HIGH)
#define IRQ_FLAGS_COMMON_LOW					(IRQ_FLAGS_TYPE_LEVEL | IRQ_FLAGS_POLARITY_HIGH)
#define IRQ_FLAGS_COMMON_RISING					(IRQ_FLAGS_TYPE_EDGE | IRQ_FLAGS_POLARITY_HIGH)
#define IRQ_FLAGS_COMMON_FALLING				(IRQ_FLAGS_TYPE_EDGE | IRQ_FLAGS_POLARITY_LOW)

#define inst_fencei() asm volatile("fence.i")

typedef void(*irq_handler_t)(void *data);

int request_irq(uint32_t irqn, irq_handler_t handler, uint32_t flags, char *name, void *data);
// int request_irq(void);
void irq_clear_pending(uint32_t irqn);
#endif
