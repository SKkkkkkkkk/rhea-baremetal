#include <stdint.h>

#define CORE_NUMS 4
volatile uint64_t secondary_cores_entry[CORE_NUMS*2];
// 0 1 | 2 3 | 4 5 | 6 7

void wakeup_core(uint8_t core_id, void* func)
{
	extern uint32_t get_core_id();
	if(core_id==get_core_id())	return;
	secondary_cores_entry[core_id*2+0] = (uint64_t)func;
	secondary_cores_entry[core_id*2+1] = 0x123456788654321;
	__asm__ volatile("dsb 0xf":::"memory");
	__asm__ volatile("sev");
	return;
}