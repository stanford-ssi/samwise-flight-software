/**
 * @author Niklas Vainio
 * @date 2025-05-27
 *
 * ADCS task for high-level ADCS control and command logic
 */

#include "adcs_task.h"
#include "adcs_driver.h"
#include "neopixel.h"
#include "pico/stdlib.h"
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

    LOG_INFO("TX COUNT {%d}", tx_count);
    if (uart_comms_packet_ready(SAMWISE_ADCS_UART))
    {
        LOG_INFO("[TELEMETRY] PACKET RECEIVED {%d}", rx_count);
        rx_count += 1;
        msg_t received;
        receive_msg(&received, rx_buf);
        switch (received.type)
        {
            case MSG_PING:
                LOG_INFO("[TELEMETRY] Ping received");
                send_pong();
                break;
            case MSG_PONG:
                LOG_INFO("[TELEMETRY] Pong received");
                // don't send ping else we get infinite loop
                break;
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
                          .dispatch_period_ms = 2000,
                          .task_init = &adcs_task_init,
                          .task_dispatch = &adcs_task_dispatch,

                          /* Set to an actual value on init */
                          .next_dispatch = 0};
