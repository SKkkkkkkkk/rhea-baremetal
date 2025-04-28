#include "16550.h"

// VIRT UART寄存器定义
#define UART0_BASE 0x10000000
#define UART_RHR (UART0_BASE + 0x00) // 接收保持寄存器
#define UART_THR (UART0_BASE + 0x00) // 发送保持寄存器
#define UART_LSR (UART0_BASE + 0x05) // 线状态寄存器
#define UART_LSR_RX_READY 0x01       // 接收数据准备好
#define UART_LSR_TX_IDLE 0x20        // 发送器空闲

void uart_init(void) {
  // QEMU的UART不需要进行额外的初始化
}

void uart_putchar(char ch) {
  volatile uint8_t *uart_lsr = (volatile uint8_t *)UART_LSR;
  volatile uint8_t *uart_thr = (volatile uint8_t *)UART_THR;

  while ((*uart_lsr & UART_LSR_TX_IDLE) == 0)
    ;
  *uart_thr = ch;
}

char uart_getchar(void) {
  volatile uint8_t *uart_lsr = (volatile uint8_t *)UART_LSR;
  volatile uint8_t *uart_rhr = (volatile uint8_t *)UART_RHR;

  while ((*uart_lsr & UART_LSR_RX_READY) == 0)
    ;
  return *uart_rhr;
}

void uart_puts(const char *str) {
  while (*str) {
    uart_putchar(*str++);
  }
}