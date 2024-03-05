#include <stdio.h>
#include "gicv3.h"
#include "dw_apb_timers.h"

int main()
{
	GIC_Discovery();
	GIC_Distributor_Init();
	GIC_Redistributor_Init();
	GIC_CPUInterfaceInit();

	void dw_apb_timer_test(bool sample);
	dw_apb_timer_test(false);
	return 0;
}