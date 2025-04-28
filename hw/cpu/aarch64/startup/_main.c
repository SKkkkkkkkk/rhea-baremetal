#ifndef VIRT
#include "arch_features.h"
#include "system_counter.h"
#include "chip.h"
#endif

void _main()
{
#ifdef FPGA
	wait_fpga_ddr_calibration_done(); //! if using ROM for code, DDR for data, this is not enough, we should've done ddr_calib even before _main();
#endif
#ifdef ENABLE_MMU
	extern void config_mmu(void);
	config_mmu();
#endif
#ifndef VIRT
	initSystemCounter(0, 0); // On VIRT Virt, there is no need to initialize the system counter.
	if(IS_IN_EL3())
		write_cntfrq_el0(getCNTFID(0));
#endif
}