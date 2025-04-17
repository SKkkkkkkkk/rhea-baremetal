#ifndef UART_H
#define UART_H

#include <stdint.h>

void uart_init(void);
void uart_putc(char ch);
void uart_puts(const char *str);
char uart_getc(void);

#endif /* UART_H */