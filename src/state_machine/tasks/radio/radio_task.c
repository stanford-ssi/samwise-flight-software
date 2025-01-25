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
#include "pins.h"
#include "slate.h"

#include "drivers/rfm9x.h"

#include "hardware/gpio.h"
#include "hardware/spi.h"

#include <string.h>

static slate_t *s;

int receive(rfm9x_t radio_module);

static void tx_done()
{
    packet_t p;
    if (queue_try_remove(&s->tx_queue, &p))
    {
        uint8_t p_buf[p.len + 4];
        p_buf[0] = p.dst;
        p_buf[1] = p.src;
        p_buf[2] = p.seq;
        p_buf[3] = p.flags;
        memcpy(p_buf + 4, &p.data[0], p.len);

        rfm9x_packet_to_fifo(&s->radio, p_buf, sizeof(p_buf));
        rfm9x_clear_interrupts(&s->radio);

        s->tx_packets++;
        s->tx_bytes += sizeof(p_buf);
    }
    else
    {
        // No more TX packets, switch to receive mode
        rfm9x_clear_interrupts(&s->radio);
        rfm9x_listen(&s->radio);
    }
}

static void rx_done()
{
    // Copy packet into receive queue and unset interrupt
    // TODO: Can we do this faster?
    uint8_t p_buf[256];
    packet_t p;

    uint8_t n = rfm9x_packet_from_fifo(&s->radio, &p_buf[0]);
    s->rx_bytes += n;

    if (n > 0)
    {
        if (n < 4)
        {
            s->rx_bad_packet_drops++;
        }
        else
        {
            p.dst = p_buf[0];
            p.src = p_buf[1];
            p.seq = p_buf[2];
            p.flags = p_buf[3];
            p.len = n - 4;
            memcpy(&p.data[0], p_buf + 4, n - 4);

            if ((p.dst == _RH_BROADCAST_ADDRESS || p.dst == s->radio_node))
            {
                s->rx_packets++;

                if (!queue_try_add(&s->rx_queue, &p))
                    s->rx_backpressure_drops++;
            }
        }
    }
    rfm9x_clear_interrupts(&s->radio);
}

static void interrupt_received(uint gpio, uint32_t events)
{
    if (gpio == RFM9X_D0)
    {

        if (rfm9x_tx_done(&s->radio))
            tx_done();
        else if (rfm9x_rx_done(&s->radio))
            rx_done();
    }
}

void radio_task_init(slate_t *slate)
{
    s = slate;

    slate->rx_bytes = 0;
    slate->rx_packets = 0;
    slate->rx_backpressure_drops = 0;
    slate->rx_bad_packet_drops = 0;

    slate->tx_bytes = 0;
    slate->tx_packets = 0;

    // transmit queue
    queue_init(&slate->tx_queue, sizeof(packet_t), TX_QUEUE_SIZE);

    // receive queue
    queue_init(&slate->rx_queue, sizeof(packet_t), RX_QUEUE_SIZE);

    // create the radio here
    slate->radio = rfm9x_mk(RFM9X_SPI, RFM9X_RESET, RFM9X_CS, RFM9X_TX,
                            RFM9X_RX, RFM9X_CLK, RFM9X_D0, &interrupt_received);

    // initialize the radio here
    rfm9x_init(&slate->radio);

    // Switch to receive mode
    rfm9x_listen(&slate->radio);

    LOG_INFO("Brought up RFM9X v%d", rfm9x_version(&slate->radio));
}

// When it sees something in the transmit queue, switches into transmit mode and
// send a packet. Otherwise, be in recieve mode. When it recieves a packet, it
// inturrupts the CPU to immediately recieve.
void radio_task_dispatch(slate_t *slate)
{
    // Switch to transmit mode if queue is not empty
    if (!queue_is_empty(&slate->tx_queue))
    {
        rfm9x_transmit(&slate->radio);
        // Since the interrupt only fires when done transmitting the last
        // packet, we need to get it started manually
        tx_done();
    }
    else
    {
        rfm9x_listen(&slate->radio);
    }
}

sched_task_t radio_task = {.name = "radio",
                           .dispatch_period_ms = 100,
                           .task_init = &radio_task_init,
                           .task_dispatch = &radio_task_dispatch,

                           /* Set to an actual value on init */
                           .next_dispatch = 0};
