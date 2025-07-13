/**
 * @author Niklas Vainio
 * @date 2025-05-27
 *
 * This code defines the picuebd side of the ADCS uart interface
 */

#include "hardware/irq.h"
#include "hardware/uart.h"
#include "pico/stdlib.h"
#include "pico/util/queue.h"
#include "pins.h"
#include "slate.h"

#include "adcs_task.h"

#define ADCS_TELEM_QUEUE_SIZE (4 * sizeof(adcs_packet_t))

// Uart parameters
#define ADCS_UART_BAUD (115200)
#define ADCS_UART_DATA_BITS (8)
#define ADCS_UART_STOP_BITS (1)
#define ADCS_UART_PARITY (UART_PARITY_NONE)

// Software configuration for SLIP, a very simple packet framing technique
// Packets start with a start byte
// Start bytes in the message are escaped with ESC + ESC_START
// Escape bytes in the message are escaped with ESC + ESC_ESC
#define SLIP_START (0xC0)
#define SLIP_ESC (0xDB)
#define SLIP_ESC_START (0xDC)
#define SLIP_ESC_ESC (0xDD)

static slate_t *slate_for_irq;
static bool escape_active = false;
static bool packet_active = false;
static uint32_t current_packet_bytes = 0;

/**
 * Simple helper function to clear a queue
 */
static void queue_clear(queue_t *q)
{
    char c;
    while (queue_try_remove(q, &c))
    {
    };
}

static void reset_packet_state()
{
    packet_active = false;
    escape_active = false;
    current_packet_bytes = 0;
    queue_clear(&slate_for_irq->adcs_telemetry_queue);

    slate_for_irq->adcs_telem_valid = false;
}

/**
 * @brief Helper function to process a byte coming in over uart
 *
 * @param byte
 */
static void handle_uart_byte(char byte)
{
    // No packet active: activate if start otherwise do nothing
    if (!packet_active)
    {
        if (byte == SLIP_START)
        {
            reset_packet_state();
            packet_active = true;
        }

        return;
    }

    // Packet is active: if this is a start byte, this signifies the end of the
    // packet
    if (byte == SLIP_START)
    {
        // Verify length of current packet - reset if not correct
        if (current_packet_bytes == sizeof(adcs_packet_t))
        {
            // Copy to slate
            for (int i = 0; i < current_packet_bytes; i++)
            {
                char c;
                if (queue_try_remove(&slate_for_irq->adcs_telemetry_queue, &c))
                {
                    ((char *)&slate_for_irq->adcs_telem)[i] = c;
                }
            }

            // Start new packet (stay active)
            current_packet_bytes = 0;
            slate_for_irq->adcs_telem_valid = true;
        }
        else
        {
            reset_packet_state();
        }

        return;
    }

    // Escape byte: set escape active flag
    if (byte == SLIP_ESC)
    {
        escape_active = true;
        return;
    }

    // Handle possible escape
    char byte_to_add = byte;

    if (escape_active)
    {
        if (byte == SLIP_ESC_ESC)
        {
            // Escaped escape
            byte_to_add = SLIP_ESC;
            escape_active = false;
        }
        else if (byte == SLIP_ESC_START)
        {
            // Escaped start
            byte_to_add = SLIP_START;
            escape_active = false;
        }
        else
        {
            // Other: reset packet
            reset_packet_state();
            return;
        }
    }

    // Otherwise, this is a normal byte - just add it to the queue
    bool result = queue_try_add(&slate_for_irq->adcs_telemetry_queue, &byte);
    current_packet_bytes++;

    if (!result)
    {
        // No space in queue: invalidate this packet
        reset_packet_state();
        return;
    }
}

/**
 * @brief IRQ function called when we receive a uart byte
 */
static void on_byte_rx()
{
    // Read all bytes
    while (uart_is_readable(SAMWISE_ADCS_UART))
    {
        uint8_t ch = uart_getc(SAMWISE_ADCS_UART);
        handle_uart_byte(ch);
    }
}

/**
 * Initialize interrupt handler for receiving from ADCS hardware
 */
void adcs_task_init(slate_t *slate)
{
    // TODO: Should probably turn on the board here using POWER_ENABLE

    // Initialize receiving queue
    queue_init(&slate->adcs_telemetry_queue, 1, ADCS_TELEM_QUEUE_SIZE);

    // Initialize the uart for receiving with an interrupt handler
    uart_init(SAMWISE_ADCS_UART, ADCS_UART_BAUD);
    gpio_set_function(
        SAMWISE_UART_TX_TO_ADCS,
        UART_FUNCSEL_NUM(SAMWISE_ADCS_UART, SAMWISE_UART_TX_TO_ADCS));
    gpio_set_function(
        SAMWISE_UART_RX_FROM_ADCS,
        UART_FUNCSEL_NUM(SAMWISE_ADCS_UART, SAMWISE_UART_RX_FROM_ADCS));

    // Set data format
    uart_set_format(SAMWISE_ADCS_UART, ADCS_UART_DATA_BITS, ADCS_UART_STOP_BITS,
                    ADCS_UART_PARITY);

    // Turn off FIFOs
    uart_set_fifo_enabled(SAMWISE_ADCS_UART, false);

    // Set up IRQ
    int UART_IRQ = SAMWISE_ADCS_UART == uart0 ? UART0_IRQ : UART1_IRQ;

    // And set up and enable the interrupt handlers
    irq_set_exclusive_handler(UART_IRQ, on_byte_rx);
    irq_set_enabled(UART_IRQ, true);

    uart_set_irq_enables(SAMWISE_ADCS_UART, true, false);

    // Bookmark slate and reset state
    slate_for_irq = slate;
    reset_packet_state();
}

void adcs_task_dispatch(slate_t *slate)
{
    // (Do not need to do anything, interrupts take care of everything)

    // TODO: should probably handle checking if the board is alive and resetting
    // if if we do not have telem every 60 seconds or someting

    // TODO: Should pack the adcs telem into main telemetry
}

sched_task_t adcs_task = {.name = "adcs",
                          .dispatch_period_ms = 60000,
                          .task_init = &adcs_task_init,
                          .task_dispatch = &adcs_task_dispatch,

                          /* Set to an actual value on init */
                          .next_dispatch = 0};