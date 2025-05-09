#include <stdio.h>
#include <stdint.h>
#include "irq.h"

void irq_handler(void)
{
    printf("irq handler\n");
}

int main()
{
    irq_set_handler(1, irq_handler);
    __nds__plic_set_priority(1, 1);
    __nds__plic_enable_interrupt(1);

    /* Enable the Machine External/Timer/Sofware interrupt bit in MIE. */
	set_csr(NDS_MIE, MIP_MEIP | MIP_MTIP | MIP_MSIP);
	/* Enable interrupts in general. */
	set_csr(NDS_MSTATUS, MSTATUS_MIE);

    __nds__plic_set_pending(1);
    while(1) printf(".");
    return 0;
}