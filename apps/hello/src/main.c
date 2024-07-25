#include <stdio.h>
#include "wakeup_core.h"
#include "arch_helpers.h"
#ifndef QEMU
#	include "chip.h"
#endif

void core1_entry()
{
	printf("hello world from core1, mpidr_el1: 0x%lx\n\r", read_mpidr_el1());
	void core2_entry();
	wakeup_core(2, core2_entry);
	while(1) __asm__ volatile("");
}

void core2_entry()
{
	printf("hello world from core2, mpidr_el1: 0x%lx\n\r", read_mpidr_el1());
	void core3_entry();
	wakeup_core(3, core3_entry);
	while(1) __asm__ volatile("");
}

void core3_entry()
{
	printf("hello world from core3, mpidr_el1: 0x%lx\n\r", read_mpidr_el1());
	while(1) __asm__ volatile("");
}

#ifndef QEMU
void chip_info()
{
	printf("*** CHIP INFO ***\n\r");
	printf("CHIP_ID: %03x\n\r", CHIP_ID);
	printf("IS_ASIC: %d\n\r", IS_ASIC);
	printf("TAPEOUT_ID: %d\n\r", TAPEOUT_ID);
	printf("MILESTONE_ID: %d\n\r", MILESTONE_ID);
	printf("CHIP_VERSION_MAJOR: %d\n\r", CHIP_VERSION_MAJOR);
	printf("CHIP_VERSION_MINOR: %d\n\r", CHIP_VERSION_MINOR);
	printf("AP_SYS_VERSION: 0x%04x\n\r", AP_SYS_VERSION);
	printf("TILE_SYS_VERSION: 0x%04x\n\r", TILE_SYS_VERSION);
	printf("D2D_SYS_VERSION: 0x%04x\n\r", D2D_SYS_VERSION);
	printf("C2C_SYS_VERSION: 0x%04x\n\r", C2C_SYS_VERSION);
}
#endif

int main()
{
#ifndef QEMU
	chip_info();
#endif
	printf("hello world from core0, mpidr_el1: 0x%lx\n\r", read_mpidr_el1());
	wakeup_core(1, core1_entry);
	return 0;
}