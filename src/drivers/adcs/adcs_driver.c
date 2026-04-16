/**
 * @author Niklas Vainio
 * @date 2025-07-20
 *
 * This code defines the hardware-level ADCS UART interface
 */

#include "hardware/uart.h"
#include "pico/stdlib.h"
#include "pins.h"
#include "slate.h"
#include <string.h>

#include "adcs_driver.h"

// Uart parameters
#define ADCS_UART_BAUD (115200)
#define ADCS_UART_DATA_BITS (8)
#define ADCS_UART_STOP_BITS (1)
#define ADCS_UART_PARITY (UART_PARITY_NONE)

// Timeout between bytes in microseconds
// currently set to 500ms (faily generous)
#define ADCS_BYTE_TIMEOUT_US (500000)

// WRITE Packet Internal Message Bytes
// TODO: change this to be equal to the largest packet data length required
// (according to specifications)
#define ACK_BYTE '!'
#define SYN_BYTE '$'
#define PACKET_CORRUPTED_BYTE '#'

#define WRITE_MAX_TIMEOUT 10000

slate_t *slate_for_irq;

// RX interrupt handler
static void uart_rx_callback()
{
    // Add characters to the queue
    while (uart_is_readable(SAMWISE_ADCS_UART))
    {
        uint8_t ch = uart_getc(SAMWISE_ADCS_UART);
        queue_try_add(&slate_for_irq->adcs_uart_queue, &ch);
    }
}

void flush_uart()
{
    if (uart_is_readable(SAMWISE_ADCS_UART))
    {
        LOG_DEBUG("ADCS UART still readable, flushing...");
        // Flush out any extra bytes that may be in the buffer
        while (uart_is_readable(SAMWISE_ADCS_UART))
        {
            char c = uart_getc(SAMWISE_ADCS_UART);
            printf("%02x ", c);
        }
        printf("\n");
    }
}

/**
 * @brief Helper function to read up to num_bytes bytes from ADCS uart with a
 * timeout in the case of missing bytes
 *
 * @param slate         Slate
 * @param buf           Buffer to read into
 * @param num_bytes     Maximum number of bytes to read
 * @param timeout_us    Timeout between bytes in microseconds
 *
 * @return Number of bytes reac successfully (between 0 and num_bytes inclusive)
 */

static uint32_t adcs_driver_read_uart_with_timeout(slate_t *slate, char *buf,
                                                   uint32_t num_bytes,
                                                   uint32_t timeout_us)
{
    printf("Attempting to read %u bytes from ADCS UART \n", num_bytes);

    absolute_time_t start = get_absolute_time();
    uint8_t *dest_ptr = (uint8_t *)buf; // Convert to char* for arithmetic
    uint32_t bytes_received = 0;
    while (absolute_time_diff_us(start, get_absolute_time()) < timeout_us)
    {
        // Drain the queue
        while (queue_try_remove(&slate->adcs_uart_queue,
                                dest_ptr + bytes_received))
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

static unsigned int crc32(const uint8_t *message, uint8_t len)
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

static bool uart_write_timeout(const uint8_t *packet, size_t len,
                               uint32_t timeout_us)
{
    uint32_t start = time_us_32();
    size_t written = 0;

    while (written < len)
    {
        // If there’s room in the FIFO, push the next byte
        if (uart_is_writable(SAMWISE_ADCS_UART))
        {
            LOG_INFO("[UArt Write Timeout: successful! Just put %x ",
                     packet[written]);
            uart_putc_raw(SAMWISE_ADCS_UART, packet[written++]);
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

size_t convert_adcs_tx_packet(uint8_t *destination, adcs_command_packet *packet)
{
    uint8_t index = 0;
    destination[index] = SYN_BYTE;
    index += 1;

    // 1 byte for the packet length
    uint8_t length = packet->packet_length;
    destination[index] = length;
    index += 1;

    // 1 byte for command
    uint8_t cmd = (uint8_t)packet->command;
    destination[index] = cmd;
    index += 1;

    // n number of bytes for the actual data.
    uint8_t iter = 0;
    while (iter < length)
    {
        destination[index] = packet->packet_data[iter];
        iter++;
        index++;
    }

    unsigned int crc_result = crc32(
        destination + 1,
        length + 2); // skip the first element of the array (the sync byte).
    // calculate crc on the length byte, cmd byte, and all the data bytes.
    memcpy(&destination[index], &crc_result,
           4); // reads the length byte, command byte, then [length] bytes for
               // the data.

    // DEBUG
    for (int i = 0; i < length + 7; i++)
    {
        LOG_DEBUG("[convert_adcs_packet], index [%d] %x ", i, destination[i]);
    }

    // return the length of the full packet.
    return length + 7;
}

adcs_result_t write_adcs_tx_packet(slate_t *slate,
                                   adcs_command_packet *adcs_packet)
{
    flush_uart();

    // packet needs to be of form uint8_t *,
    uint8_t packet[MAX_DATA_BYTES + 7];
    uint8_t data_length = convert_adcs_tx_packet(packet, adcs_packet);

    // Check packet length
    if (data_length > MAX_DATA_BYTES + 7)
    {
        LOG_DEBUG("Packet is too long!\n");
        return ADCS_WRITE_PACKET_TOO_BIG;
    }
    // Send actual packet

    if (!uart_write_timeout(packet, data_length, WRITE_MAX_TIMEOUT))
    {
        return ADCS_WRITE_TIMEDOUT;
    }

    absolute_time_t start = get_absolute_time();

    while (absolute_time_diff_us(start, get_absolute_time()) <
           10000) // we wait 10ms for our response
    {
        // our interrupt handler will already be loading the data into the
        // queue.
        uint8_t receivedByte;
        if (queue_try_remove(&slate->adcs_uart_queue, &receivedByte))
        {
            if (receivedByte == ACK_BYTE)
            {
                return ADCS_WRITE_SUCCESS;
            }
            else if (receivedByte == PACKET_CORRUPTED_BYTE)
            {
                return ADCS_WRITE_CORRUPTED;
            }
        }
    }
    return ADCS_WRITE_TIMEDOUT;
}

adcs_result_t adcs_driver_init(slate_t *slate)
{
    if (!slate)
    {
        return ADCS_ERROR_INVALID_PARAM;
    }

    slate_for_irq = slate; // store a copy of the slate pointer.

    // Initialize power pins
    gpio_init(SAMWISE_ADCS_EN);
    gpio_set_dir(SAMWISE_ADCS_EN, GPIO_OUT);

    // Power on the board by default
    adcs_driver_power_on(slate);

    // Initialize uart for sending commands/receiving telemetry
    uart_init(SAMWISE_ADCS_UART, ADCS_UART_BAUD);
    gpio_init(SAMWISE_UART_TX_TO_ADCS);
    gpio_init(SAMWISE_UART_RX_FROM_ADCS);
    gpio_set_function(
        SAMWISE_UART_TX_TO_ADCS,
        UART_FUNCSEL_NUM(SAMWISE_ADCS_UART, SAMWISE_UART_TX_TO_ADCS));
    gpio_set_function(
        SAMWISE_UART_RX_FROM_ADCS,
        UART_FUNCSEL_NUM(SAMWISE_ADCS_UART, SAMWISE_UART_RX_FROM_ADCS));

    // Set data format
    uart_set_format(SAMWISE_ADCS_UART, ADCS_UART_DATA_BITS, ADCS_UART_STOP_BITS,
                    ADCS_UART_PARITY);

    // Set up a RX interrupt
    // We need to set up the handler first
    // Select correct interrupt for the UART we are using
    int UART_IRQ = SAMWISE_ADCS_UART == uart1 ? UART1_IRQ : UART0_IRQ;
    queue_init(&slate->adcs_uart_queue,
               sizeof(uint8_t), /* Size of each element */
               256 /* Max elements */);

    // And set up and enable the interrupt handlers
    irq_set_exclusive_handler(UART_IRQ, uart_rx_callback);
    irq_set_enabled(UART_IRQ, true);

    // Now enable the UART to send interrupts - RX only
    uart_set_irq_enables(SAMWISE_ADCS_UART, true, false);

    return ADCS_SUCCESS;
}

adcs_result_t adcs_driver_power_on(slate_t *slate)
{
    // Set power enable high to turn on the board and allow time to settle
    slate->is_adcs_on = true;
    gpio_put(SAMWISE_ADCS_EN, 1);
    sleep_ms(10);

    return ADCS_SUCCESS;
}

adcs_result_t adcs_driver_power_off(slate_t *slate)
{
    // Set power enable low to turn on the board and allow time to settle
    slate->is_adcs_on = false;
    gpio_put(SAMWISE_ADCS_EN, 0);
    sleep_ms(10);

    return ADCS_SUCCESS;
}

adcs_result_t adcs_driver_get_telemetry(slate_t *slate, adcs_packet_t *packet)
{
    if (!slate || !packet)
    {
        return ADCS_ERROR_INVALID_PARAM;
    }

    // Flush any existing data in the UART buffer
    flush_uart();

    adcs_command_packet telemetry_packet;
    memset(&telemetry_packet, 0, sizeof(telemetry_packet));
    telemetry_packet.command = SEND_TELEM;
    telemetry_packet.packet_length = 0;

    // Send a ping to the ADCS board and read back telemetry
    LOG_INFO("[ACDS] Putting command to ADCS UART: %c", ADCS_SEND_TELEM);
    // uart_putc_raw(SAMWISE_ADCS_UART, ADCS_SEND_TELEM);
    write_adcs_tx_packet(slate, &telemetry_packet);

    uint32_t num_bytes_read = adcs_driver_read_uart_with_timeout(
        slate, (char *)packet, sizeof(adcs_packet_t), ADCS_BYTE_TIMEOUT_US);

    slate->is_adcs_telem_valid = (num_bytes_read == sizeof(adcs_packet_t));

    LOG_INFO("[ADCS] state: %02x", packet->state);
    LOG_INFO("[ADCS] boot_count: %u", packet->boot_count);

    for (int i = 0; i < sizeof(adcs_packet_t); ++i)
    {
        LOG_INFO("[ADCS] packet[%d]: %02x", i, ((char *)packet)[i]);
    }

    if (!slate->is_adcs_telem_valid)
    {
        LOG_ERROR("[ADCS] Failed to read full telemetry packet, "
                  "expected %zu bytes, got %u bytes",
                  sizeof(adcs_packet_t), num_bytes_read);
        return ADCS_ERROR_UART_FAILED;
    }

    // Return true if we received all expected bytes
    return slate->is_adcs_telem_valid ? ADCS_SUCCESS : ADCS_ERROR_UART_FAILED;
}

bool adcs_driver_is_alive(slate_t *slate)
{
    if (!slate)
    {
        return ADCS_ERROR_INVALID_PARAM;
    }

    // Return immediately if board is off
    if (!slate->is_adcs_on)
    {
        return false;
    }

    // Flush any existing data in the UART buffer
    flush_uart();

    adcs_command_packet health_packet;
    memset(&health_packet, 0, sizeof(health_packet));
    health_packet.command = HEALTH_CHECK;
    health_packet.packet_length = 0;

    // Send a ping to the ADCS board and expect to read back a known byte
    LOG_INFO("[ADCS] Sending health check command: %c\n", ADCS_HEALTH_CHECK);
    // uart_putc_raw(SAMWISE_ADCS_UART, ADCS_HEALTH_CHECK);
    write_adcs_tx_packet(slate, &health_packet);

    char c;
    uint32_t num_bytes_read = adcs_driver_read_uart_with_timeout(
        slate, &c, sizeof(char), ADCS_BYTE_TIMEOUT_US);

    LOG_INFO("[ADCS] Received %u bytes: %02x (%c) %s\n", num_bytes_read, c, c,
             c == ADCS_HEALTH_CHECK_SUCCESS ? "HEALTHY" : "FAILED");

    // Return true if we received a byte, and it is the expected value
    return (num_bytes_read > 0) && (c == ADCS_HEALTH_CHECK_SUCCESS);
}