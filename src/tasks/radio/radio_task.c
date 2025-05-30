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
        p_buf[0] = p.dst;
        p_buf[1] = p.src;
        p_buf[2] = p.seq;
        p_buf[3] = p.flags;
        memcpy(p_buf + 4, &p.data[0], p.len);

        LOG_INFO("p size: %d", sizeof(p_buf));
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

static bool parse_packet(uint8_t *p_buf, uint8_t n, packet_t *p)
{
    const uint8_t min_packet_size = sizeof(packet_t) - sizeof(p->data);
    if (n < min_packet_size)
        return false;

    uint8_t offset = 0;
    p->dst = p_buf[offset++];
    p->src = p_buf[offset++];
    p->flags = p_buf[offset++];
    p->seq = p_buf[offset++];
    p->len = p_buf[offset++];

    uint8_t data_len = p->len;
    if (data_len > sizeof(p->data))
        return false;

    memcpy(p->data, p_buf + offset, data_len);
    offset += data_len;
    memcpy(&p->boot_count, p_buf + offset, 4);
    offset += 4;
    memcpy(&p->msg_id, p_buf + offset, 4);
    offset += 4;
    memcpy(p->hmac, p_buf + offset, TC_SHA256_DIGEST_SIZE);
    return true;
}

static void rx_done()
{
    uint8_t p_buf[256];
    packet_t p;

    uint8_t n = rfm9x_packet_from_fifo(&s->radio, p_buf);
    s->rx_bytes += n;

    if (!parse_packet(p_buf, n, &p))
    {
        s->rx_bad_packet_drops++;
        rfm9x_clear_interrupts(&s->radio);
        return;
    }

    if ((p.dst == _RH_BROADCAST_ADDRESS || p.dst == s->radio_node))
    {
        s->rx_packets++;
        if (!queue_try_add(&s->rx_queue, &p))
            s->rx_backpressure_drops++;
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
    rfm9x_set_tx_irq(&slate->radio, &tx_done);
    // rfm9x_set_tx_irq(&slate->radio, 0);
    rfm9x_set_rx_irq(&slate->radio, &rx_done);

    // Switch to receive mode
    rfm9x_listen(&slate->radio);
    // rfm9x_transmit(&slate->radio);
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
        LOG_INFO("Transmitting...");
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
