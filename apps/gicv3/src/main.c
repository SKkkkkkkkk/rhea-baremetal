#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "gicv3.h"
#include "arch_helpers.h"

void irq0_handler(void)
{
	printf("This is irq0\n\r");
}
void irq29_handler(void)
{
	printf("This is irq29\n\r");
}
void irq32_handler(void)
{
	printf("This is irq32\n\r");
}

int main(void)
{
	GIC_Distributor_Init();
	GIC_Redistributor_Init();
	GIC_CPUInterfaceInit();

	GIC_EnableIRQ(0);
	GIC_EnableIRQ(29);
	GIC_EnableIRQ(32);

	IRQ_SetHandler(0, irq0_handler);
	IRQ_SetHandler(29, irq29_handler);
	IRQ_SetHandler(32, irq32_handler);

	GIC_SetPendingIRQ(0);
	GIC_SetPendingIRQ(29);
	GIC_SetPendingIRQ(32);

	while(1) __asm__ volatile("");
	return 1;
}
