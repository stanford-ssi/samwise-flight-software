/**
 * @author  Joseph Shetaye
 * @date    2024-11-18
 *
 * Task to drive the main radio. This task will keep the radio in receive mode,
 * and switch to transmit mode to send packets on the transmit queue.
 */

#include "radio_task.h"

static slate_t *s;

static void tx_done()
{
    packet_t p;
    if (queue_try_remove(&s->tx_queue, &p))
    {
        uint8_t p_buf[p.len + 4];
        p_buf[0] = DST_ADDR;
        p_buf[1] = SRC_ADDR;
        p_buf[2] = 0; // p.seq;
        p_buf[3] = 0; // p.flags;
        memcpy(p_buf + 4, &p.data[0], p.len);

        rfm9x_packet_to_fifo(&s->radio, p_buf, sizeof(p_buf));
        //rfm9x_clear_interrupts(&s->radio);

        s->tx_packets++;
        s->tx_bytes += sizeof(p_buf);

	LOG_INFO("about to transmit...");
	rfm9x_transmit(&s->radio);
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

    // Install interrupt handlers

    rfm9x_set_tx_irq(&slate->radio, NULL);
    //rfm9x_set_rx_irq(&slate->radio, &rx_done);

    // Power up!
    rfm9x_power_up(&slate->radio);

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
        // Since the interrupt only fires when done transmitting the last
        // packet, we need to get it started manually
        tx_done();
    }
    else
    {
      //rfm9x_listen(&slate->radio);
    }
}

sched_task_t radio_task = {.name = "radio",
                           .dispatch_period_ms = 1000,
                           .task_init = &radio_task_init,
                           .task_dispatch = &radio_task_dispatch,

                           /* Set to an actual value on init */
                           .next_dispatch = 0};
