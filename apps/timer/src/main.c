#include <stdio.h>
#include "gicv3.h"
#include "dw_apb_timers.h"
#include "systimer.h"

int main()
{
	GIC_Distributor_Init();
	GIC_Redistributor_Init();
	GIC_CPUInterfaceInit();

#if 1
	void dw_apb_timer_test(bool sample);
	dw_apb_timer_test(false);
#else
	systimer_init();
	uint32_t i = 0;
	while(1)
	{
		printf("%u\n",++i);
		systimer_delay(1, IN_S);
	}
#endif
	
	return 0;
}