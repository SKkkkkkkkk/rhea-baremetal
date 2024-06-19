#include <stdio.h>
#include "chip.h"
#include "arch_helpers.h"
#include "xmodem.h"

#define  FWU_SRAM_BASE		APRAM_BASE
#define  FWU_SRAM_MAX_SIZE	(512*1024U - 128*1024U)

static void memcpy_to_fwu_sram_base(unsigned char *buf, int buflen)
{
	static uint32_t size = 0;
	memcpy((void *)(uintptr_t)(FWU_SRAM_BASE + size), buf, buflen);
	size += buflen;
}

static void setpc(uintptr_t pc) 
{
	assert((pc&3)==0);
	dmbsy();
	dsbsy();
	isb();
	disable_mmu_icache_elx();
	/* Flush all caches. */
	dcsw_op_all(DCCISW);
	invalidate_icache_all();
	asm volatile ("br %0"::"r"(pc):"memory");
	UNREACHABLE();
}

int main()
{
	printf ("NS_BL1U: Receiving FWU_SRAM...\n");
	int st;
	while( (st = xmodemReceiveWithAction(memcpy_to_fwu_sram_base, FWU_SRAM_MAX_SIZE)) < 0)
		printf ("NS_BL1U: Xmodem receive error: status: %d, retrying...\n", st);
	printf("NS_BL1U: Xmodem successfully received %d bytes\n", st);
	printf("NS_BL1U: Jumping to FWU_SRAM(PC=0x%x)...\n", FWU_SRAM_BASE);
	setpc(FWU_SRAM_BASE);
	UNREACHABLE();
}