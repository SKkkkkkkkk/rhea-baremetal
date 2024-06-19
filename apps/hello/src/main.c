#include <stdio.h>
#include "wakeup_core.h"
#include "chip.h"

void core1_entry()
{
	printf("hello world from core1\n\r");
	void core2_entry();
	wakeup_core(2, core2_entry);
	while(1) __asm__ volatile("");
}

void core2_entry()
{
	printf("hello world from core2\n\r");
	void core3_entry();
	wakeup_core(3, core3_entry);
	while(1) __asm__ volatile("");
}

void core3_entry()
{
	printf("hello world from core3\n\r");
	while(1) __asm__ volatile("");
}

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

int main()
{
	chip_info();
	printf("hello world from core0\n\r");
	wakeup_core(1, core1_entry);
	return 0;
}