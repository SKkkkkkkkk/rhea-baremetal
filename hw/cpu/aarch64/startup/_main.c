#ifndef QEMU
#include "arch_features.h"
#include "system_counter.h"
#endif

void _main()
{
#ifdef ENABLE_MMU
	extern void config_mmu(void);
	config_mmu();
#endif
#ifndef QEMU
	initSystemCounter(0, 0); // On QEMU Virt, there is no need to initialize the system counter.
	if(IS_IN_EL3())
		write_cntfrq_el0(getCNTFID(0));
#endif
}