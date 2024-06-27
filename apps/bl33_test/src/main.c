#include <stdio.h>
#include <stdint.h>
#include "time_stamp.h"
#include "chip.h"

int main()
{
	TIME_STAMP();
	printf("Hello World From NS World!\n\r");
	system_reset();
	return 0;
}