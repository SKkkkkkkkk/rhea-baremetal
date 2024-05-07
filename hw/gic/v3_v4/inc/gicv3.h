#ifndef __GICV3_H__
#define __GICV3_H__

#include <stdbool.h>
#include <stdint.h>

void GIC_Distributor_Init();

void GIC_Redistributor_Init();

void GIC_CPUInterfaceInit();

void GIC_SetRouting(uint16_t int_id, uint32_t affinity, bool irm);

uint32_t GIC_GetRouting(uint16_t int_id);

void GIC_EnableIRQ(uint16_t int_id);

bool GIC_IsIRQEnabled(uint16_t int_id);

void GIC_DisableIRQ(uint16_t int_id);

bool GIC_GetPendingIRQ(uint16_t int_id);

void GIC_SetPendingIRQ(uint16_t int_id);

void GIC_ClearPendingIRQ(uint16_t int_id);

void GIC_SetEdgeOrLevel(uint16_t int_id, bool is_level_triggered);

bool GIC_IsLevelSensitive(uint16_t int_id);

void GIC_SetPriority(uint16_t int_id, uint8_t priority);

uint8_t GIC_GetPriority(uint16_t int_id);

typedef enum
{
	G0S  = 0U,
	G1NS = 1U,
	G1S  = 2U,
} GICv3_Goroup_t;

void GIC_SetGroup(uint16_t int_id, GICv3_Goroup_t group);

GICv3_Goroup_t GIC_GetGroup(uint16_t int_id);

void GIC_Init();

typedef void (*IRQHandler_t) (void);

void IRQ_SetHandler (uint16_t int_id, IRQHandler_t handler);
IRQHandler_t IRQ_GetHandler (uint16_t int_id);

#endif // __GICV3_H__