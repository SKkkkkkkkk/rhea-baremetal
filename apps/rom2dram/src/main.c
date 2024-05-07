#include <stdio.h>
#include "wakeup_core.h"
#include "memmap.h"

static inline void jump2xxx(uint64_t pc)
{
	// Ensure all instructions before the jump are completed
	__asm__ volatile("dsb ish");

	// Invalidate instruction cache
	__asm__ volatile("ic iallu");

	// Branch to new address
	__asm__ volatile("br %0" : : "r" (pc));
	
	__builtin_unreachable();
}

void core1_entry()
{
	void core2_entry();
	wakeup_core(2, core2_entry);

	printf("core1 about to jump to dram...\n\r");
	jump2xxx(AP_DRAM_BASE);

	__builtin_unreachable();
}

void core2_entry()
{
	void core3_entry();
	wakeup_core(3, core3_entry);

	printf("core2 about to jump to dram...\n\r");
	jump2xxx(AP_DRAM_BASE);

	__builtin_unreachable();
}

void core3_entry()
{
	printf("core3 about to jump to dram...\n\r");
	jump2xxx(AP_DRAM_BASE);
	__builtin_unreachable();
}

int main()
{
	wakeup_core(1, core1_entry);
	printf("core0 about to jump to dram...\n\r");
	jump2xxx(AP_DRAM_BASE);
	return 0;
}