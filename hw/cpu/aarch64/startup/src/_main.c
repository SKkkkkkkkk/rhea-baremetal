#ifndef QEMU
#include "arch_features.h"
#include "system_counter.h"
#endif

void _main()
{
#ifndef QEMU
	initSystemCounter(0, 0); // On QEMU Virt, there is no need to initialize the system counter.
	write_cntfrq_el0(getCNTFID(0));
#endif
}