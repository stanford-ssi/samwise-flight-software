#include "rfm9x.h"
#include "rfm9x_spi.h"
#include "rfm9x_registers.h"

/*
 * Packet helpers
 */

/*
 * Put a packet into the FIFO
 *
 * len must be less than PACKET_SIZE
 */
uint8_t rfm9x_packet_to_fifo(rfm9x_t *r, uint8_t *buf, uint8_t n)
{
    if (n > PACKET_SIZE)
        return 0;

    rfm9x_set_fifo_address_pointer(r, rfm9x_get_tx_base_address(r));
    rfm9x_put_buf(r, _RH_RF95_REG_00_FIFO, buf, n);
    rfm9x_put8(r, _RH_RF95_REG_22_PAYLOAD_LENGTH, n);

    return n;
}

/*
 * Get a packet from the FIFO
 */
uint8_t rfm9x_packet_from_fifo(rfm9x_t *r, uint8_t *buf)
{
    uint8_t n = rfm9x_get_rx_bytes(r);
    if (n > PACKET_SIZE)
        return 0;

    rfm9x_set_fifo_address_pointer(r, rfm9x_get_fifo_address_pointer(r));
    rfm9x_get_buf(r, _RH_RF95_REG_00_FIFO, buf, n);
    rfm9x_put8(r, _RH_RF95_REG_22_PAYLOAD_LENGTH, 0); /* ?? */
    return n;
}

/*
 * Send a raw transmission from the RFM9X.
 *
 * r: the radio
 * data: the data to send
 * l: the length of the data. Must be less than `PAYLOAD_SIZE`
 * keep_listening: 0 to stop listening after sending, 1 to keep blocking
 * destination: radio to send it to. 255 is broadcast.
 * node: our address
 * identifier: Sequence number — if sending multiple packets, increment by one
 * per packet.
 * flags:
 */
uint8_t rfm9x_send(rfm9x_t *r, char *data, uint32_t l, uint8_t keep_listening,
                   uint8_t destination, uint8_t node, uint8_t identifier,
                   uint8_t flags)
{
    if (l > PAYLOAD_SIZE)
    {
        if (r->debug)
            printf("[rfm9x] Oversize packet to send: %ld\\r\\n", l);
        return 0;
    }

    uint8_t buf[PACKET_SIZE];
    buf[0] = destination;
    buf[1] = node;
    buf[2] = identifier;
    buf[3] = flags;
    memcpy(buf + 4, data, l);

    uint8_t packet_len = l + 4;

    if (r->debug)
    {
        printf("[rfm9x] Sending to %d from %d, id %d, flags 0x%02x:\\r\\n",
               destination, node, identifier, flags);
        rfm9x_print_packet(buf, packet_len);
    }

    rfm9x_set_mode(r, STANDBY_MODE);
    rfm9x_packet_to_fifo(r, buf, packet_len);
    rfm9x_set_mode(r, TX_MODE);

    if (keep_listening)
    {
        while (!rfm9x_tx_done(r))
        {
            // We should have a timeout
            sleep_ms(10);
        }
        rfm9x_clear_interrupts(r);
        rfm9x_set_mode(r, RX_MODE);
    }

    return packet_len;
}

/*
 * Send a transmission.
 *
 * Sends l bytes of data, tagged with the current seq number. Waits for an ack.
 *
 * Returns 1 if an ack was received, 0 otherwise.
 */
uint8_t rfm9x_send_ack(rfm9x_t *r, char *data, uint32_t l, uint8_t destination,
                       uint8_t node, uint8_t max_retries)
{
    uint8_t acked = 0;
    uint8_t retries = 0;
    uint8_t flags = _SAP_FLAGS_ACK_REQUEST;
    uint8_t ack_buffer[PACKET_SIZE];

    if (r->debug)
        printf("[rfm9x] Sending ACK request, max retries %d\\r\\n", max_retries);

    while (!acked && retries < max_retries)
    {
        rfm9x_send(r, data, l, 0, /* Don't keep listening, we do that */
                   destination, node, r->seq, flags);

        // If set we must've sent a retry
        if (flags & _RH_FLAGS_RETRY)
            retries++;

        if (destination == _RH_BROADCAST_ADDRESS)
        {
            if (r->debug)
                printf("[rfm9x] Skipping ACK for broadcast...\\r\\n");
            // No ack for broadcast
            acked = 1;
        }
        else
        {
            // Wait for ack
            uint16_t l = rfm9x_receive(r, ack_buffer, node, 0, 0, 1);
            if (l > 0)
            { /* Received something */
                if (ack_buffer[3] & _RH_FLAGS_ACK)
                { /* Was an ACK */
                    if (ack_buffer[2] == r->seq)
                    { /* Was an ACK for this message */
                        acked = 1;
                    }
                    else
                    {
                        if (r->debug)
                            printf("[rfm9x] Not for this message\\r\\n");
                    }
                }
                else
                {
                    if (r->debug)
                        printf("[rfm9x] Not an ACK\\r\\n");
                }
            }
        }

        // If we didn't receive an ACK, wait to retransmit
        if (!acked)
        {
            sleep_ms(100);
            flags |= _RH_FLAGS_RETRY;
            if (r->debug)
                printf("[rfm9x] Retransmitting seq %d, retry count %d\\r\\n", r->seq,
                       retries);
        }
    }

    if (acked)
        r->seq++;
    return acked;
}

/*
 * Receive a transmission.
 */
uint8_t rfm9x_receive(rfm9x_t *r, char *packet, uint8_t node,
                      uint8_t keep_listening, uint8_t with_ack,
                      bool blocking_wait_for_packet)
{
    uint8_t buf[PACKET_SIZE];
    uint8_t packet_len = 0;
    uint8_t destination;
    uint8_t source;
    uint8_t identifier;
    uint8_t flags;

    if (r->debug)
        printf("[rfm9x] Receiving, waiting for packet...\\r\\n");

    if (blocking_wait_for_packet)
    {
        while (!rfm9x_rx_done(r))
        {
            // We should have a timeout
            sleep_ms(10);
        }
    }
    else
    {
        if (!rfm9x_rx_done(r))
        {
            if (r->debug)
                printf("[rfm9x] No packet available\\r\\n");
            return 0;
        }
    }

    rfm9x_clear_interrupts(r);
    rfm9x_set_mode(r, STANDBY_MODE);
    packet_len = rfm9x_packet_from_fifo(r, buf);

    destination = buf[0];
    source = buf[1];
    identifier = buf[2];
    flags = buf[3];
    memcpy(packet, buf + 4, packet_len - 4);

    if (r->debug)
    {
        printf("[rfm9x] Received from %d to %d, id %d, flags 0x%02x:\\r\\n", source,
               destination, identifier, flags);
        rfm9x_print_packet(buf, packet_len);
    }

    if (destination != node && destination != _RH_BROADCAST_ADDRESS)
    {
        if (r->debug)
            printf("[rfm9x] Not for this node\\r\\n");
        packet_len = 0;
    }

    if (with_ack && destination != _RH_BROADCAST_ADDRESS)
    {
        uint8_t ack_packet[4];
        ack_packet[0] = source;      /* Destination */
        ack_packet[1] = node;        /* Source */
        ack_packet[2] = identifier;  /* Identifier */
        ack_packet[3] = _RH_FLAGS_ACK; /* Flags */
        rfm9x_set_mode(r, STANDBY_MODE);
        rfm9x_packet_to_fifo(r, ack_packet, 4);
        rfm9x_set_mode(r, TX_MODE);
        while (!rfm9x_tx_done(r))
        {
            // We should have a timeout
            sleep_ms(10);
        }
        rfm9x_clear_interrupts(r);
        if (r->debug)
            printf("[rfm9x] Sent ACK\\r\\n");
    }

    if (keep_listening)
        rfm9x_set_mode(r, RX_MODE);

    return packet_len;
}

