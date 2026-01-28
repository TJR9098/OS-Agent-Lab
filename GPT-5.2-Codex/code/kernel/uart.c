#include <stdint.h>

#include "memlayout.h"
#include "uart.h"

#define UART_RBR 0x00
#define UART_THR 0x00
#define UART_IER 0x01
#define UART_FCR 0x02
#define UART_LCR 0x03
#define UART_LSR 0x05

#define LSR_THRE 0x20
#define LSR_DR 0x01

static inline void uart_write(uint32_t reg, uint8_t val) {
  *(volatile uint8_t *)(UART0_BASE + reg) = val;
}

static inline uint8_t uart_read(uint32_t reg) {
  return *(volatile uint8_t *)(UART0_BASE + reg);
}

void uart_init(void) {
  uart_write(UART_IER, 0x00);
  uart_write(UART_LCR, 0x80);
  uart_write(UART_RBR, 0x01);
  uart_write(UART_IER, 0x00);
  uart_write(UART_LCR, 0x03);
  uart_write(UART_FCR, 0x07);
}

void uart_putc(char c) {
  if (c == '\n') {
    uart_putc('\r');
  }

  while ((uart_read(UART_LSR) & LSR_THRE) == 0) {
  }
  uart_write(UART_THR, (uint8_t)c);
}

void uart_puts(const char *s) {
  while (*s) {
    uart_putc(*s++);
  }
}

int uart_getc(void) {
  if ((uart_read(UART_LSR) & LSR_DR) == 0) {
    return -1;
  }
  return (int)uart_read(UART_RBR);
}

int uart_rx_ready(void) {
  return (uart_read(UART_LSR) & LSR_DR) != 0;
}
