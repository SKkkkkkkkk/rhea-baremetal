#include <stdio.h>
#include <stdint.h>
#include "memmap.h"

int main()
{
	printf("Hello World From NS World!\n\r");
	*(volatile uint32_t*)(SYSCTRL_CFG_BASE + 0x400) = 0;
	return 0;
}