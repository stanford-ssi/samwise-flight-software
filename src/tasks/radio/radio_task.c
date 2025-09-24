/**
 * @author  Joseph Shetaye
 * @date    2024-11-18
 *
 * Task to drive the main radio. This task will keep the radio in receive mode,
 * and switch to transmit mode to send packets on the transmit queue.
 */

#include "radio_task.h"
#include "neopixel.h"

static slate_t *s;

// --- PACKET ENCODER/DECODER ---
// p.len is always the length of p.data (payload), not including header fields.

// Serializes a packet_t into a buffer. Returns the total number of bytes
// written, or 0 on error.
static size_t encode_packet(const packet_t *p, uint8_t *buf, size_t bufsize,
                            bool enable_hmac)
{
    if (!p || !buf)
    {
        LOG_ERROR("encode_packet: NULL pointer provided");
        return 0;
    }
    if (p->len > PACKET_DATA_SIZE)
    {
        LOG_ERROR("encode_packet: Packet length exceeds maximum data size");
        return 0;
    }

    size_t total_size;
    if (__builtin_add_overflow(PACKET_HEADER_SIZE, p->len, &total_size) ||
        (enable_hmac &&
         __builtin_add_overflow(total_size, PACKET_FOOTER_SIZE, &total_size)))
    {
        LOG_ERROR("encode_packet: Integer overflow in total size calculation");
        return 0; // Integer overflow would occur
    }

    if (bufsize < total_size)
    {
        LOG_ERROR("encode_packet: Buffer size too small for packet");
        return 0;
    }

    size_t offset = 0;

    // Write header fields
    buf[offset++] = p->dst;
    buf[offset++] = p->src;
    buf[offset++] = p->flags;
    buf[offset++] = p->seq;
    buf[offset++] = p->len;

    memcpy(buf + offset, p->data, p->len);
    offset += p->len;

    if (enable_hmac) // Typically our downlink is not authenticated
    {
        memcpy(buf + offset, &p->boot_count, sizeof(p->boot_count));
        offset += sizeof(p->boot_count);
        memcpy(buf + offset, &p->msg_id, sizeof(p->msg_id));
        offset += sizeof(p->msg_id);
        memcpy(buf + offset, p->hmac, PACKET_HMAC_SIZE);
        offset += PACKET_HMAC_SIZE;
    }

    return offset;
}

// Parses a buffer into a packet_t. Returns true on success, false on error.
static bool parse_packet(const uint8_t *buf, size_t n, packet_t *p)
{
    if (!buf || !p)
    {
        LOG_ERROR("parse_packet: NULL pointer provided");
        return false;
    }
    if (n < PACKET_MIN_SIZE)
    {
        LOG_ERROR("parse_packet: Buffer too small for a valid packet, "
                  "PACKET_MIN_SIZE: %u",
                  PACKET_MIN_SIZE);
        return false;
    }

    size_t offset = 0;

    // Parse header fields
    p->dst = buf[offset++];
    p->src = buf[offset++];
    p->flags = buf[offset++];
    p->seq = buf[offset++];
    p->len = buf[offset++];

    LOG_INFO("parse_packet [RECEIVED PACKER]: dst=%d, src=%d, flags=0x%02x, "
             "seq=%d, len=%d",
             p->dst, p->src, p->flags, p->seq, p->len);

    if (p->len > PACKET_DATA_SIZE)
    {
        LOG_ERROR("parse_packet: Packet length exceeds maximum data size, "
                  "PACKET_DATA_SIZE: %u",
                  PACKET_DATA_SIZE);
        return false;
    }

    size_t total_required_size;
    if (__builtin_add_overflow(PACKET_HEADER_SIZE, p->len,
                               &total_required_size) ||
        __builtin_add_overflow(total_required_size, PACKET_FOOTER_SIZE,
                               &total_required_size))
    {
        LOG_ERROR("parse_packet: Integer overflow in total size calculation");
        return false; // Integer overflow would occur
    }

    if (n != total_required_size)
    {
        LOG_ERROR("parse_packet: expect packet size %zu, got buffer size %zu",
                  total_required_size, n);
        return false;
    }

    memcpy(p->data, buf + offset, p->len);
    offset += p->len;
    memcpy(&p->boot_count, buf + offset, sizeof(p->boot_count));
    offset += sizeof(p->boot_count);
    memcpy(&p->msg_id, buf + offset, sizeof(p->msg_id));
    offset += sizeof(p->msg_id);
    memcpy(p->hmac, buf + offset, PACKET_HMAC_SIZE);
    offset += PACKET_HMAC_SIZE;

    return true;
}

// --- TX ---
static void tx_done()
{
    packet_t p = {0};
    if (queue_try_remove(&s->tx_queue, &p))
    {
        uint8_t p_buf[PACKET_SIZE];
        size_t pkt_size = encode_packet(&p, p_buf, sizeof(p_buf), false);
        if (pkt_size == 0)
        {
            LOG_ERROR("Failed to encode packet for TX");
            rfm9x_clear_interrupts(&s->radio);
            return;
        }
        rfm9x_packet_to_fifo(&s->radio, p_buf, pkt_size);
        rfm9x_clear_interrupts(&s->radio);
        s->tx_packets++;
        s->tx_bytes += pkt_size;
    }
    else
    {
        // No more TX packets, switch to receive mode
        rfm9x_clear_interrupts(&s->radio);
        rfm9x_listen(&s->radio);
    }
}

// --- RX ---
static void rx_done()
{
    uint8_t p_buf[PACKET_SIZE] = {0};
    packet_t p = {0};
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

    // Print out the LoRA parameters
    rfm9x_print_parameters(&slate->radio);
    LOG_INFO("Brought up RFM9X v%d", rfm9x_version(&slate->radio));
    LOG_INFO("  Node: %d", &slate->radio_node);
}

// When it sees something in the transmit queue, switches into transmit mode and
// send a packet. Otherwise, be in recieve mode. When it recieves a packet, it
// inturrupts the CPU to immediately recieve.
void radio_task_dispatch(slate_t *slate)
{
    neopixel_set_color_rgb(RADIO_TASK_COLOR);
    // Switch to transmit mode if queue is not empty
    if (!queue_is_empty(&slate->tx_queue))
    {
        // Check if transmission is already in progress to avoid race condition
        if (rfm9x_tx_done(&slate->radio))
        {
            rfm9x_transmit(&slate->radio);
            LOG_INFO("Transmitting...");
            // Since the interrupt only fires when done transmitting the last
            // packet, we need to get it started manually
            tx_done();
        }
        // If transmission is in progress, do nothing and let the interrupt
        // handle it
    }
    else
    {
        rfm9x_listen(&slate->radio);
    }
    neopixel_set_color_rgb(0, 0, 0);
}

sched_task_t radio_task = {.name = "radio",
                           .dispatch_period_ms = 100,
                           .task_init = &radio_task_init,
                           .task_dispatch = &radio_task_dispatch,

                           /* Set to an actual value on init */
                           .next_dispatch = 0};
