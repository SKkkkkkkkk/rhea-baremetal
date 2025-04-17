#include <stdio.h>
#include <stdlib.h>
#include "dw_apb_wdt.h"
#include "systimer.h"
#include "gicv3.h"
#include "common.h"
// #include "irq_ctrl.h"
// #include "chip_mem_layout.h"
// #include "regs_type.h"

#define WDT_IRQn 38

void wdt_irqhandler(void)
{
	printf("in irq.\n\r");
	wdt_clear_irq(WDT_ID);
	// wdt_feed(WDT_ID);
}


int main()
{
	GIC_Init();

	printf("wdt test.\n\r");

	systimer_init();

	wdt_reset(WDT_ID);

	IRQ_SetHandler(WDT_IRQn, wdt_irqhandler);
	GIC_SetPriority(WDT_IRQn, 0 << 3);
	GIC_EnableIRQ(WDT_IRQn);


	wdt_setup(WDT_ID, 0, 1, 0xa);
	uint32_t i = 0;
	while(1)
	{
		printf("%u\n\r", i++);
		systimer_delay(1, IN_S);
	}
	return 0;
}

