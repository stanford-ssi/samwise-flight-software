/**
 * @author Niklas Vainio
 * @date 2025-05-27
 *
 * ADCS task for high-level ADCS control and command logic
 */

#include "adcs_task.h"
#include "adcs_driver.h"
#include "hardware/gpio.h"
#include "neopixel.h"
#include "pico/stdlib.h"
#include "pins.h"
#include "slate.h"

#include "cobs.h"
#include "protocol.h"
#include "uart_communications.h"

#define ADCS_MAX_FAILED_CHECKS_BEFORE_REBOOT (5)

void adcs_task_init(slate_t *slate)
{

    uart_comms_init(SAMWISE_ADCS_UART, SAMWISE_UART_TX_TO_ADCS,
                    SAMWISE_UART_RX_FROM_ADCS, 115200);
    // adcs_driver_init();
    gpio_init(SAMWISE_ADCS_EN);
    gpio_set_dir(SAMWISE_ADCS_EN, GPIO_OUT);

    // slate->adcs_num_failed_checks = 0;
    // adcs_driver_power_on();
}

static uint32_t tx_count;
static uint32_t rx_count;
static uint8_t rx_buf[256];

void adcs_task_dispatch(slate_t *slate)
{
    neopixel_set_color_rgb(ADCS_TASK_COLOR);

    // Turn on adcs_pin after init for some reason
    // TODO: figure out why this breaks the code if it happens during init
    sleep_ms(100);
    gpio_put(SAMWISE_ADCS_EN, 1);
    sleep_ms(100);

    LOG_INFO("[ADCS] TX COUNT {%d}", tx_count);
    if (uart_comms_packet_ready(SAMWISE_ADCS_UART))
    {
        LOG_INFO("[ADCS] PACKET RECEIVED {%d}", rx_count);
        rx_count += 1;
        msg_t received;
        uint32_t num_bytes = receive_msg(&received, rx_buf);
        switch (received.type)
        {
            case MSG_PING:
                if (num_bytes != 8)
                    break;
                LOG_INFO("[ADCS] Ping received");
                send_pong();
                break;
            case MSG_PONG:
                if (num_bytes != 8)
                    break;
                LOG_INFO("[ADCS] Pong received");
                // don't send ping else we get infinite loop
                slate->is_adcs_on = true;
                break;
            case MSG_ADCS_PACKET:
                if (num_bytes != 7 + sizeof(adcs_packet_t))
                    LOG_INFO("[ADCS] Packet dropped, invalid size");
                ;
                LOG_INFO("[ADCS] Attitude packet received");
                slate->is_adcs_on = true;
                slate->adcs_telemetry = *(adcs_packet_t *)received.payload;
                adcs_print_telemetry(&slate->adcs_telemetry);
        } // end switch
    }
    else
    {
        // SEND PING MESSAGE
        tx_count += 1;
        send_ping();
    }

    neopixel_set_color_rgb(0, 0, 0);
}

sched_task_t adcs_task = {.name = "adcs",
                          .dispatch_period_ms = 1000,
                          .task_init = &adcs_task_init,
                          .task_dispatch = &adcs_task_dispatch,

                          /* Set to an actual value on init */
                          .next_dispatch = 0};
