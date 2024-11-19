/**
 * @author  Joseph Shetaye
 * @date    2024-11-18
 *
 * Task to drive the main radio. This task will keep the radio in receive mode,
 * and switch to transmit mode to send packets on the transmit queue.
 */

#include "state_machine/tasks/radio_task.h"
#include "macros.h"
#include "pico/stdlib.h"
#include "pico/util/queue.h"
#include "slate.h"
#include "pins.h"

#include "hardware/spi.h"

void radio_task_init(slate_t *slate)
{
  queue_init(&slate->tx_queue, sizeof(packet_t), TX_QUEUE_SIZE);
  queue_init(&slate->rx_queue, sizeof(packet_t), RX_QUEUE_SIZE);

  slate->radio = rfm9x_mk(RFM9X_SPI, RFM9X_RESET, RFM9X_CS, RFM9X_TX, RFM9X_RX, RFM9X_CLK);
  rfm9x_init(&slate->radio);
  printf("Brought up RFM9X v%d", rfm9x_version(&slate->radio));
}

void radio_task_dispatch(slate_t *slate)
{
}

sched_task_t radio_task = {.name = "radio",
                           .dispatch_period_ms = 1000,
                           .task_init = &radio_task_init,
                           .task_dispatch = &radio_task_dispatch,

                           /* Set to an actual value on init */
                           .next_dispatch = 0};
