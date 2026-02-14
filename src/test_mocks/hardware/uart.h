#pragma once

#include <stdbool.h>
#include <stdint.h>

// Mock UART types
typedef struct
{
    uint8_t dummy;
} uart_inst_t;

#define uart0 ((uart_inst_t *)0)
#define uart1 ((uart_inst_t *)1)

static inline void uart_init(uart_inst_t *uart, unsigned int baudrate)
{
}
static inline void uart_set_hw_flow(uart_inst_t *uart, bool cts, bool rts)
{
}
static inline void uart_set_format(uart_inst_t *uart, unsigned int data_bits,
                                   unsigned int stop_bits, unsigned int parity)
{
}
static inline void uart_set_fifo_enabled(uart_inst_t *uart, bool enabled)
{
}
static inline bool uart_is_readable(uart_inst_t *uart)
{
    return false;
}
static inline uint8_t uart_getc(uart_inst_t *uart)
{
    return 0;
}
static inline void uart_putc(uart_inst_t *uart, char c)
{
}
static inline void uart_puts(uart_inst_t *uart, const char *s)
{
}
