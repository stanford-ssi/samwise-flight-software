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
#include "hardware/gpio.h"

const uint RADIO_INTERRUPT_PIN = 28;
const bool ENABLE_IRQ = true;

static slate_t* s;

int receive(rfm9x_t radio_module);

void interrupt_recieved(uint gpio, uint32_t events)
{
    LOG_INFO("Interrupt received on pin %d\n", gpio);
    if (gpio == RADIO_INTERRUPT_PIN)
    {
        LOG_INFO("Radio interrupt received\n");
        //receive(radio_module);

        receive(s->radio);
    }
}

void radio_task_init(slate_t *slate)
{
  s = slate;
  gpio_init(RADIO_INTERRUPT_PIN);
  gpio_set_dir(RADIO_INTERRUPT_PIN, GPIO_IN);
  gpio_pull_down(RADIO_INTERRUPT_PIN);

  // transmit queue
  queue_init(&slate->tx_queue, sizeof(packet_t), TX_QUEUE_SIZE);

  // receive queue
  queue_init(&slate->rx_queue, sizeof(packet_t), RX_QUEUE_SIZE);

  // create the radio here
  slate->radio = rfm9x_mk(RFM9X_SPI, RFM9X_RESET, RFM9X_CS, RFM9X_TX, RFM9X_RX, RFM9X_CLK);

  // initialize the radio here
  rfm9x_init(&slate->radio);

  gpio_set_irq_enabled_with_callback(RADIO_INTERRUPT_PIN, GPIO_IRQ_EDGE_RISE,
                                       ENABLE_IRQ, &interrupt_recieved);

  LOG_INFO("Brought up RFM9X v%d", rfm9x_version(&slate->radio));
}


// When it sees something in the transmit queue, switches into recieve mode and
// send a packet. Otherwise, be in recieve mode. When it recieves a packet, it
// inturrupts the CPU to immediately recieve.
void radio_task_dispatch(slate_t *slate)
{

}

// screen /dev/tty.usbmodem1101
int receive(rfm9x_t radio_module)
{
  char data[256];
    uint8_t n = rfm9x_receive(&radio_module, &data[0], 1, 0, 0, 1);
    printf("Received %d\n", n);

    bool interruptPin = gpio_get(RADIO_INTERRUPT_PIN);
    printf("Interrupt pin: %d\n", interruptPin);
}

sched_task_t radio_task = {.name = "radio",
                           .dispatch_period_ms = 1000,
                           .task_init = &radio_task_init,
                           .task_dispatch = &radio_task_dispatch,

                           /* Set to an actual value on init */
                           .next_dispatch = 0};
