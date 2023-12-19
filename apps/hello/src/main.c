#include <stdio.h>
#include "wakeup_core.h"
#include "component1/inc/componet1.h"

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

int main()
{
	component1();
	printf("hello world from core0\n\r");
	wakeup_core(1, core1_entry);
	return 0;
}