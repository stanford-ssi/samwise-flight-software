/**
 * @author  Joseph Shetaye
 * @date    2024-11-18
 *
 * Task to drive the main radio. This task will keep the radio in receive mode,
 * and switch to transmit mode to send packets on the transmit queue.
 */

#include "radio_task.h"
#include "logger.h"
#include "macros.h"
#include "pico/stdlib.h"
#include "pico/util/queue.h"
#include "pins.h"
#include "rfm9x.h"
#include "slate.h"

#include "hardware/spi.h"

#define SPI0_CLK (18)
#define SPI0_TX (19)
#define SPI0_RX (16)

void radio_task_init(slate_t *slate)
{
    queue_init(&slate->tx_queue, sizeof(packet_t), TX_QUEUE_SIZE);
    queue_init(&slate->rx_queue, sizeof(packet_t), RX_QUEUE_SIZE);

    slate->radio = rfm9x_mk(SPI_INSTANCE(SAMWISE_RF_SPI), SAMWISE_RF_RST_PIN,
                            SAMWISE_RF_CS_PIN, SAMWISE_RF_MOSI_PIN,
                            SAMWISE_RF_MISO_PIN, SAMWISE_RF_SCK_PIN);
    rfm9x_init(&slate->radio);
    printf("Brought up RFM9X v%d", rfm9x_version(&slate->radio));
}

void radio_task_dispatch(slate_t *slate)
{
    //    rfm9x_send(&slate->radio, "HELLO WORLD", 11, false, 0xff, 0xff, 0, 0);
    LOG_INFO("sending packet...");
}

sched_task_t radio_task = {.name = "radio",
                           .dispatch_period_ms = 1000,
                           .task_init = &radio_task_init,
                           .task_dispatch = &radio_task_dispatch,

                           /* Set to an actual value on init */
                           .next_dispatch = 0};
