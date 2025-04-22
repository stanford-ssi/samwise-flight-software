#ifndef RFM9X_PACKET_H_
#define RFM9X_PACKET_H_

#include "rfm9x.h"

/*
 * Packet helpers
 */

/*
 * Put a packet into the FIFO
 *
 * len must be less than PACKET_SIZE
 */
uint8_t rfm9x_packet_to_fifo(rfm9x_t *r, uint8_t *buf, uint8_t n);

/*
 * Get a packet from the FIFO
 */
uint8_t rfm9x_packet_from_fifo(rfm9x_t *r, uint8_t *buf);

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
                   uint8_t flags);

/*
 * Send a transmission.
 *
 * Sends l bytes of data, tagged with the current seq number. Waits for an ack.
 *
 * Returns 1 if an ack was received, 0 otherwise.
 */
uint8_t rfm9x_send_ack(rfm9x_t *r, char *data, uint32_t l, uint8_t destination,
                       uint8_t node, uint8_t max_retries);

/*
 * Receive a transmission.
 */
uint8_t rfm9x_receive(rfm9x_t *r, char *packet, uint8_t node,
                      uint8_t keep_listening, uint8_t with_ack,
                      bool blocking_wait_for_packet);

#endif /* RFM9X_PACKET_H_ */
