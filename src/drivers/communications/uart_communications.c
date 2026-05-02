/*
 * Author:      @Carson Lauer
 * Date:        April 26, 2026
 * Description: Uart communication library.
 * This library includes a irq_callback to tx uart messages from a buffer
 * and to receive uart messages into a buffer. It assumes either COBS
 * encoded packets or null-terminated strings (both end in an 0x00 char).
 */

#include "uart_communications.h"
#include "hardware/irq.h"
#include "hardware/uart.h"
#include "macros.h"
#include "pico/printf.h"
#include "pico/stdlib.h"

#define UART_RX_BUFFER_SIZE 256
#define UART_TX_BUFFER_SIZE 256

#define DATA_BITS (8)
#define STOP_BITS (1)
#define PARITY (UART_PARITY_NONE)

// RX ring buffer: head = write (IRQ), tail = read (application)
static uint8_t rx_buffer[2][UART_RX_BUFFER_SIZE];
static volatile uint16_t rx_head[2] = {0, 0};
static volatile uint16_t rx_tail[2] = {0, 0};

// TX ring buffer: head = write (application), tail = read (IRQ)
static uint8_t tx_buffer[2][UART_TX_BUFFER_SIZE];
static volatile uint16_t tx_head[2] = {0, 0};
static volatile uint16_t tx_tail[2] = {0, 0};

// ── internal helpers ────────────────────────────────────────────────────────

static inline uint16_t rx_count(uint8_t idx)
{
    return (rx_head[idx] - rx_tail[idx] + UART_RX_BUFFER_SIZE) %
           UART_RX_BUFFER_SIZE;
}

static inline uint16_t tx_count(uint8_t idx)
{
    return (tx_head[idx] - tx_tail[idx] + UART_TX_BUFFER_SIZE) %
           UART_TX_BUFFER_SIZE;
}

static inline uint16_t tx_available(uint8_t idx)
{
    return UART_TX_BUFFER_SIZE - 1 - tx_count(idx);
}

// Peek at a byte at offset `offset` from rx_tail without consuming it
static inline uint8_t rx_peek(uint8_t idx, uint16_t offset)
{
    return rx_buffer[idx][(rx_tail[idx] + offset) % UART_RX_BUFFER_SIZE];
}

// ── IRQ drain helpers (called from both IRQ handlers) ───────────────────────

static void uart_rx_isr(uint8_t idx, uart_inst_t *uart)
{
    while (uart_is_readable(uart))
    {
        uint16_t next = (rx_head[idx] + 1) % UART_RX_BUFFER_SIZE;
        if (next == rx_tail[idx])
        {
            // Buffer full — drop byte (better than corrupting the ring)
            (void)uart_getc(uart);
        }
        else
        {
            rx_buffer[idx][rx_head[idx]] = uart_getc(uart);
            rx_head[idx] = next;
        }
    }
}

static void uart_tx_isr(uint8_t idx, uart_inst_t *uart)
{
    while (uart_is_writable(uart) && tx_tail[idx] != tx_head[idx])
    {
        uart_putc_raw(uart, tx_buffer[idx][tx_tail[idx]]);
        tx_tail[idx] = (tx_tail[idx] + 1) % UART_TX_BUFFER_SIZE;
    }
    // Nothing left — disable TX IRQ to stop spurious firings
    if (tx_tail[idx] == tx_head[idx])
    {
        uart_set_irq_enables(uart, true, false);
    }
}

// ── public API ───────────────────────────────────────────────────────────────

void uart_comms_init(uart_inst_t *uart_instance, uint8_t tx, uint8_t rx,
                     uint32_t baud)
{

    uint8_t idx = uart_get_index(uart_instance);

    uart_init(uart_instance, baud);

    // gpio_init(tx);
    gpio_set_function(tx, GPIO_FUNC_UART);
    // gpio_init(rx);
    gpio_set_function(rx, GPIO_FUNC_UART);
    gpio_set_input_enabled(rx, true);

    uart_set_hw_flow(uart_instance, false, false);
    uart_set_fifo_enabled(uart_instance, false);
    uart_set_format(uart_instance, DATA_BITS, STOP_BITS, PARITY);

    rx_head[idx] = rx_tail[idx] = 0;
    tx_head[idx] = tx_tail[idx] = 0;

    int irq_num = (idx == 0) ? UART0_IRQ : UART1_IRQ;
    LOG_INFO("uart initializing uart%d", idx);

    irq_set_priority(irq_num, 0x60);
    if (idx == 0)
    {
        irq_set_exclusive_handler(irq_num, uart0_irq_handler);
    }
    else
    {
        irq_set_exclusive_handler(irq_num, uart1_irq_handler);
    }

    // Only RX enabled at init — TX enabled on demand in uart_comms_tx
    uart_set_irq_enables(uart_instance, true, false);
    irq_set_enabled(irq_num, true);
}

uint16_t uart_comms_tx(uart_inst_t *uart_instance, uint8_t *data,
                       uint16_t length)
{

    uint8_t idx = uart_get_index(uart_instance);
    uint16_t avail = tx_available(idx);

    if (length > avail)
    {
        length = avail;
    }

    for (uint16_t i = 0; i < length; i++)
    {
        tx_buffer[idx][tx_head[idx]] = data[i];
        tx_head[idx] = (tx_head[idx] + 1) % UART_TX_BUFFER_SIZE;
    }

    // If UART is writable right now, kick off transmission manually
    // This is necessary — TX IRQ only fires AFTER the first byte is written
    if (uart_is_writable(uart_instance) && tx_tail[idx] != tx_head[idx])
    {
        uart_putc_raw(uart_instance, tx_buffer[idx][tx_tail[idx]]);
        tx_tail[idx] = (tx_tail[idx] + 1) % UART_TX_BUFFER_SIZE;
    }

    // Enable TX IRQ to drain the rest
    uart_set_irq_enables(uart_instance, true, true);

    return length;
}

uint16_t uart_comms_packet_ready(uart_inst_t *uart_instance)
{
    uint8_t idx = uart_get_index(uart_instance);
    uint16_t count = rx_count(idx);

    for (uint16_t offset = 0; offset < count; offset++)
    {
        if (rx_peek(idx, offset) == 0x00)
        {
            return offset + 1; // includes the terminating 0x00
        }
    }
    return 0;
}

uint16_t uart_comms_get_packet(uart_inst_t *uart_instance, uint8_t *buffer,
                               uint16_t max_length)
{

    uint8_t idx = uart_get_index(uart_instance);
    uint16_t packet_length = uart_comms_packet_ready(uart_instance);

    if (packet_length == 0)
    {
        return 0;
    }
    if (packet_length > max_length)
    {
        // Packet won't fit — discard it entirely so we don't get stuck
        for (uint16_t i = 0; i < packet_length; i++)
        {
            rx_tail[idx] = (rx_tail[idx] + 1) % UART_RX_BUFFER_SIZE;
        }
        return 0;
    }

    for (uint16_t i = 0; i < packet_length; i++)
    {
        buffer[i] = rx_buffer[idx][rx_tail[idx]];
        rx_tail[idx] = (rx_tail[idx] + 1) % UART_RX_BUFFER_SIZE;
    }

    return packet_length;
}

uint16_t uart_comms_rx_count(uart_inst_t *uart_instance)
{
    uint8_t idx = uart_get_index(uart_instance);
    return rx_count(idx);
}

// ── IRQ handlers ─────────────────────────────────────────────────────────────

void uart0_irq_handler(void)
{
    uart_rx_isr(0, uart0);
    uart_tx_isr(0, uart0);
}

void uart1_irq_handler(void)
{
    uart_rx_isr(1, uart1);
    uart_tx_isr(1, uart1);
}
