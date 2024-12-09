#ifndef __CONFIG_H__
#define __CONFIG_H__

#define UART_CONSOLE		1
#define UART_CLCI		0
#define UART_CONSOLE_BAUDRATE	115200
#define UART_CLCI_BAUDRATE	115200

#define CLCI_CONFOG_LOAD_ADDRESS	0x10017000 // 4KB

#define DOORBELL_MODE_ISR    0 /*0:polling mode; 1:ISR mode*/
#define UART_MODE_ISR        0 /*0:polling mode; 1:ISR mode*/

#endif
