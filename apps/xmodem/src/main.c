#include <stdio.h>
#include <string.h>
#include "xmodem.h"

#if 1

#define R_BUF_ADDR 0xa0000000U
#define R_BUF_SIZE (100*1024*1024)
int main(void)
{
	int st;

	printf ("Send data using the xmodem protocol from your terminal emulator now...\n");
	memset((void *)R_BUF_ADDR, 0, R_BUF_SIZE);
	/* the following should be changed for your environment:
	   0x30000 is the download address,
	   65536 is the maximum size to be written at this address
	 */
	st = xmodemReceive((void *)R_BUF_ADDR, R_BUF_SIZE);
	if (st < 0) {
		printf ("Xmodem receive error: status: %d\n", st);
	}
	else  {
		printf ("Xmodem successfully received %d bytes\n", st);
	}
	while(1);
    return 0;
}

#else

#include "pl011.h"
int main(void)
{
	while(1)
	{
		char c = uart_getchar();
		uart_putchar(c);
	}
	return 0;
}

#endif
