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

#include "adcs_driver.h"

// Uart parameters
#define ADCS_UART_BAUD (115200)
#define ADCS_UART_DATA_BITS (8)
#define ADCS_UART_STOP_BITS (1)
#define ADCS_UART_PARITY (UART_PARITY_NONE)

// Sentinel bytes for commands
#define ADCS_SEND_TELEM ('T')
#define ADCS_HEALTH_CHECK ('?')
#define ADCS_HEALTH_CHECK_SUCCESS ('!')

// Timeout between bytes in microseconds
// currently set to 500ms (faily generous)
#define ADCS_BYTE_TIMEOUT_US (500000)

/**
 * @brief Helper function to read up to num_bytes bytes from ADCS uart with a
 * timeout in the case of missing bytes
 *
 * @param buf           Buffer to read into
 * @param num_bytes     Maximum number of bytes to read
 * @param timeout_us    Timeout between bytes in microseconds
 *
 * @return Number of bytes reac successfully (between 0 and num_bytes inclusive)
 */

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

static uint32_t adcs_driver_read_uart_with_timeout(char *buf,
                                                   uint32_t num_bytes,
                                                   uint32_t timeout_us)
{
    printf("Attempting to read %u bytes from ADCS UART", num_bytes);
    for (uint32_t i = 0; i < num_bytes; i++)
    {
        if (uart_is_readable_within_us(SAMWISE_ADCS_UART, timeout_us))
        {
            buf[i] = uart_getc(SAMWISE_ADCS_UART);
        }
        else
        {
            return i;
        }
    }

    return num_bytes;
}

adcs_result_t adcs_driver_init(slate_t *slate)
{
    if (!slate)
    {
        return ADCS_ERROR_INVALID_PARAM;
    }

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

    // Send a ping to the ADCS board and read back telemetry
    LOG_INFO("[ACDS] Putting command to ADCS UART: %c", ADCS_SEND_TELEM);
    uart_putc_raw(SAMWISE_ADCS_UART, ADCS_SEND_TELEM);

    uint32_t num_bytes_read = adcs_driver_read_uart_with_timeout(
        (char *)packet, sizeof(adcs_packet_t), ADCS_BYTE_TIMEOUT_US);

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

    // Send a ping to the ADCS board and expect to read back a known byte
    LOG_INFO("[ADCS] Sending health check command: %c\n", ADCS_HEALTH_CHECK);
    uart_putc_raw(SAMWISE_ADCS_UART, ADCS_HEALTH_CHECK);

    char c;
    uint32_t num_bytes_read = adcs_driver_read_uart_with_timeout(
        &c, sizeof(char), ADCS_BYTE_TIMEOUT_US);

    LOG_INFO("[ADCS] Received %u bytes: %02x (%c) %s\n", num_bytes_read, c, c,
             c == ADCS_HEALTH_CHECK_SUCCESS ? "HEALTHY" : "FAILED");

    // Return true if we received a byte, and it is the expected value
    return (num_bytes_read > 0) && (c == ADCS_HEALTH_CHECK_SUCCESS);
}