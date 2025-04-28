#ifndef UART_H
#define UART_H

#include <stdint.h>

void uart_init(void);
void uart_putchar(char ch);
void uart_puts(const char *str);
char uart_getchar(void);

#endif /* UART_H */