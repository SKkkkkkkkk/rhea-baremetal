#include "delay.h"

#define XMODEM_RECEIVE_ONLY

#ifdef VIRT
#include "pl011.h"
#define xmodem_putchar(x) 	uart_putchar(x)
#define xmodem_getchar() 	uart_getchar()
#define xmodem_tstc() 		uart_tstc()
#else
#include "dw_apb_uart.h"
#define xmodem_putchar(x) 	uart_sendchar(SEEHI_UART0, x)
#define xmodem_getchar() 	uart_getchar(SEEHI_UART0)
#define xmodem_tstc() 		uart_tstc(SEEHI_UART0)
#endif

int _inbyte(uint64_t timeout) // usec timeout
{
	uint64_t start_count_val = syscounter_read();
	uint64_t wait_cycles = (timeout * read_cntfrq_el0()) / 1000000;
	while ((syscounter_read() - start_count_val) < wait_cycles) {
		if (xmodem_tstc())
			return xmodem_getchar();
	}
	return -1;
}

void _outbyte(int c)
{
	xmodem_putchar(c);
}
