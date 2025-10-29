/**
 * @author Niklas Vainio and Marc Reyes
 * @date 2025-01-18
 *
 * UART packet and command handler routines for communicating with the RPi
 */
#include "hardware/irq.h"
#include "hardware/uart.h"
#include "pico/stdlib.h"
#include "pico/util/queue.h"

#include "macros.h"
#include "payload_uart.h"
#include "pins.h"
#include "slate.h"

#include "safe_sleep.h"

#define PAYLOAD_UART_ID uart0 // Required to use pins 30 and 31 (see datasheet)

// UART parameters
#define BAUD_RATE 115200
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY UART_PARITY_NONE
#define WRITE_MAX_TIMEOUT 10000
#define MAX_WRITE_TRIES 3

// Packet parameters
#define MAX_PACKET_LEN 4069
#define ACK_BYTE '!'
#define SYN_RETRIES 3
#define SYN_BYTE '$'
#define SYN_COUNT 3
#define START_TRIES 10
#define START_BYTE '@'

static slate_t *slate_for_irq; // Need to save to be accessible to IRQ

// Note: The start byte ensures that desyncing does not happen.
typedef struct
{
    uint16_t length;   // NOTE: Little endian
    uint16_t seq_num;  // NOTE: Little endian
    uint32_t checksum; // NOTE: Little endian
} packet_header_t;

// RX interrupt handler
static void uart_rx_callback()
{
    // Add characters to the queue
    while (uart_is_readable(PAYLOAD_UART_ID))
    {
        uint8_t ch = uart_getc(PAYLOAD_UART_ID);
        queue_try_add(&slate_for_irq->rpi_uart_queue, &ch);
        slate_for_irq->rpi_uart_last_byte_receive_time = get_absolute_time();
    }
}

// From https://gist.github.com/xobs/91a84d29152161e973d717b9be84c4d0
// (not using fast version because we want small binary size)
unsigned int crc32(const uint8_t *message, uint16_t len)
{
    size_t i;
    unsigned int byte, crc, mask;

    i = 0;
    crc = 0xFFFFFFFF;
    while (i < len)
    {
        byte = message[i]; // Get next byte.
        crc = crc ^ byte;
        for (int j = 0; j < 8; j++)
        { // Do eight times.
            mask = -(crc & 1);
            crc = (crc >> 1) ^ (0xEDB88320 & mask);
        }
        i = i + 1;
    }
    return ~crc;
}

/**
 * Return the header for a given message packet
 */
static packet_header_t compute_packet_header(const uint8_t *packet,
                                             uint16_t len, uint16_t seq_num)
{
    packet_header_t header = {len, seq_num, crc32(packet, len)};

    return header;
}

/**
 * Receive up to num_bytes bytes into the buffer specified by dest.
 *
 * @return Number of bytes received, may be less than desired
 */
static uint16_t receive_into(slate_t *slate, void *dest, uint16_t num_bytes,
                             uint32_t timeout_ms)
{
    absolute_time_t start = get_absolute_time();
    uint8_t *dest_ptr = (uint8_t *)dest; // Convert to char* for arithmetic
    uint16_t bytes_received = 0;
    while (absolute_time_diff_us(start, get_absolute_time()) <
           timeout_ms * 1000)
    {
        // Drain the queue
        while (
            queue_try_remove(&slate->rpi_uart_queue, dest_ptr + bytes_received))
        {
            bytes_received++;
            if (bytes_received == num_bytes)
            {
                return num_bytes;
            }
        }
    }

    return bytes_received;
}

static bool receive_ack(slate_t *slate)
{
    // Receive a single byte
    uint8_t received_byte;
    uint16_t received = receive_into(slate, &received_byte, 1, 1000);

    return received && received_byte == ACK_BYTE;
}

static bool receive_syn(slate_t *slate)
{
    // Receive multiple sync bytes
    uint8_t count = 0;

    while (true)
    {
        uint8_t received_byte;
        uint16_t received = receive_into(slate, &received_byte, 1, 1000);

        if (!received)
            return false;

        if (received_byte == SYN_BYTE)
        {
            count++;
        }

        if (count >= SYN_COUNT)
        {
            return true;
        }
    }
}

static bool receive_header_start(slate_t *slate)
{
    uint8_t run_count = 0;

    while (run_count < START_TRIES)
    {
        uint8_t received_byte;
        uint16_t received = receive_into(slate, &received_byte, 1, 1000);

        if (!received)
        {
            LOG_DEBUG("No bytes received!\n");
            return false;
        }

        if (received_byte == START_BYTE)
        {
            return true;
        }

        LOG_DEBUG("Wrong byte received, rerunning to get start byte!\n");
        run_count++;
    }

    LOG_DEBUG("Wait for start header timeout!\n");
    return false;
}

static void send_ack()
{
    uart_putc_raw(PAYLOAD_UART_ID, ACK_BYTE);
}

static void send_syn()
{
    for (size_t i = 0; i < SYN_COUNT; i++)
    {
        uart_putc_raw(PAYLOAD_UART_ID, SYN_BYTE);
    }
}

void payload_turn_on(slate_t *slate)
{
    gpio_put(SAMWISE_RPI_ENAB, 1);
    slate->is_payload_on = true;
}

void payload_turn_off(slate_t *slate)
{
    // To prevent from writing and hanging
    if (!slate->is_payload_on)
    {
        return;
    }

    char packet[] = "[\"shutdown\",  [], {\"immediate\"}]";
    payload_uart_write_packet(slate, packet, (sizeof(packet) - 1), 0);

    // NOTE: This does not actually turn off the payload, this just ensures it
    // doesn't turn on again when it is turned off.
    gpio_put(SAMWISE_RPI_ENAB, 0);

    slate->is_payload_on = false;
}

/**
 * Initialize hardware and interrupts
 */
bool payload_uart_init(slate_t *slate)
{
    // Store slate pointer
    slate_for_irq = slate;

    // Set the TX and RX pins by using the function select on the GPIO
    gpio_set_function(SAMWISE_UART_TX,
                      UART_FUNCSEL_NUM(PAYLOAD_UART_ID, SAMWISE_UART_TX));
    gpio_set_function(SAMWISE_UART_RX,
                      UART_FUNCSEL_NUM(PAYLOAD_UART_ID, SAMWISE_UART_RX));

    // Setting RPI_ENABLE_PIN as output
    gpio_init(SAMWISE_RPI_ENAB);
    gpio_set_dir(SAMWISE_RPI_ENAB, GPIO_OUT);
    payload_turn_off(slate);

    // Set baud rate
    uart_init(PAYLOAD_UART_ID, BAUD_RATE);

    // Set UART flow control CTS/RTS, we don't want these, so turn them off
    uart_set_hw_flow(PAYLOAD_UART_ID, false, false);

    // Set our data format
    uart_set_format(PAYLOAD_UART_ID, DATA_BITS, STOP_BITS, PARITY);

    // Turn off FIFO's - we want to do this character by character
    uart_set_fifo_enabled(PAYLOAD_UART_ID, false);

    // Set up a RX interrupt
    // We need to set up the handler first
    // Select correct interrupt for the UART we are using
    int UART_IRQ = PAYLOAD_UART_ID == uart0 ? UART0_IRQ : UART1_IRQ;
    queue_init(&slate->rpi_uart_queue,
               sizeof(uint8_t), /* Size of each element */
               256 /* Max elements */);

    // And set up and enable the interrupt handlers
    irq_set_exclusive_handler(UART_IRQ, uart_rx_callback);
    irq_set_enabled(UART_IRQ, true);

    // Now enable the UART to send interrupts - RX only
    uart_set_irq_enables(PAYLOAD_UART_ID, true, false);

    return true;
}

/**
 * Writes data via UART with a timeout, ensuring that our writes do not
 * block infinitely long.
 *
 * @param packet        Array of bytes containing the packet
 * @param len           Number of bytes in the packet
 * @param timeout_us    Time microseconds that the command will try to write
 * until doing a timeout
 */
bool uart_write_timeout(const uint8_t *packet, size_t len, uint32_t timeout_us)
{
    uint32_t start = time_us_32();
    size_t written = 0;

    while (written < len)
    {
        // If there’s room in the FIFO, push the next byte
        if (uart_is_writable(PAYLOAD_UART_ID))
        {
            uart_putc_raw(PAYLOAD_UART_ID, packet[written++]);
        }
        // Else, check if we’ve run out of time
        else if (time_us_32() - start >= timeout_us)
        {
            return false; // timeout
        }
        // Otherwise, spin until either writable or timeout
    }

    return true; // all bytes enqueued within time budget
}

/**
 * Write a packet to the RPi over UART
 *
 * @param packet    Array of bytes containing the packet
 * @param len       Number of bytes in the packet
 * @param seq_num   Sequence number (useful for sending multiple packets, ignore
 * otherwise)
 */
payload_write_error_code payload_uart_write_packet(slate_t *slate,
                                                   const uint8_t *packet,
                                                   uint16_t len,
                                                   uint16_t seq_num)
{
    // Check packet length
    if (len > MAX_PACKET_LEN)
    {
        LOG_DEBUG("Packet is too long!\n");
        return PACKET_TOO_BIG;
    }

    // Write sync packet and wait for ack
    bool syn_acknowledged = false;
    for (size_t i = 0; i < SYN_RETRIES; i++)
    {
        for (size_t j = 0; j < SYN_COUNT; j++)
        {
            uart_putc_raw(PAYLOAD_UART_ID, SYN_BYTE);
        }

        if (receive_ack(slate))
        {
            syn_acknowledged = true;
            break;
        }

        safe_sleep_ms(100);
    }

    if (!syn_acknowledged)
    {
        LOG_DEBUG("Payload did not respond to sync!\n");
        return SYN_UNSUCCESSFUL;
    }

    uart_putc_raw(PAYLOAD_UART_ID, START_BYTE);

    // Calculate the header
    packet_header_t header = compute_packet_header(packet, len, seq_num);

    // Send header and receive ACK
    if (!uart_write_timeout((uint8_t *)&header, sizeof(packet_header_t),
                            WRITE_MAX_TIMEOUT))
    {
        return UART_WRITE_TIMEDOUT;
    }

    if (!receive_ack(slate))
    {
        LOG_DEBUG("Header was not acknowledged!\n");
        return HEADER_UNACKNOWLEDGED;
    }

    // Send actual packet
    uart_write_timeout(packet, len, WRITE_MAX_TIMEOUT);

    // Wait for ACK
    if (receive_ack(slate))
    {
        return SUCCESSFUL_WRITE;
    }
    return FINAL_WRITE_UNSUCCESSFUL;
}

/**
 * Read a serial packet from the RPi
 *
 * @param packet    Array of bytes to place the packet
 * @return The number of bytes read, or 0 if no response
 */
uint16_t payload_uart_read_packet(slate_t *slate, uint8_t *packet)
{
    uint16_t bytes_received;

    // Wait for sync
    if (!receive_syn(slate))
    {
        LOG_DEBUG("Syn was not received!\n");
        return 0;
    }
    send_ack();

    // Just wait for a start byte before reading the header
    if (!receive_header_start(slate))
    {
        return 0;
    }

    // Receive header
    packet_header_t header;
    bytes_received =
        receive_into(slate, &header, sizeof(packet_header_t), 50);

    if (bytes_received < sizeof(packet_header_t))
    {
        LOG_DEBUG("Header was not received!\n");
        return 0;
    }
    send_ack();

    // Check header
    if (header.length > MAX_PACKET_LEN)
    {
        LOG_DEBUG("Packet is too long!\n");
        return 0;
    }

    // Read actual packet
    bytes_received = receive_into(slate, packet, header.length, 50);

    if (bytes_received < header.length)
    {
        LOG_DEBUG("Packet was not fully received!\n");
        return 0;
    }

    // Verify checksum
    if (crc32(packet, header.length) != header.checksum)
    {
        LOG_DEBUG("Invalid checksum!\n");
        return 0;
    }
    send_ack();

    return header.length;
}
