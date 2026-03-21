#include "rfm9x.h"

void rfm9x_print_parameters(rfm9x_t *r)
{
    // TODO: Log parameters for test debugging
}
uint32_t rfm9x_version(rfm9x_t *r)
{
    return 0x12;
}
uint8_t rfm9x_tx_done(rfm9x_t *r)
{
    // TODO: Allow tests to simulate TX completion state
    return 1;
}
void rfm9x_transmit(rfm9x_t *r)
{
    // TODO: Capture transmitted data for test assertions
}
void rfm9x_listen(rfm9x_t *r)
{
    // TODO: Track listen state for test assertions
}
void rfm9x_clear_interrupts(rfm9x_t *r)
{
    // TODO: Track interrupt state for test assertions
}
uint8_t rfm9x_packet_to_fifo(rfm9x_t *r, uint8_t *buf, uint8_t n)
{
    // TODO: Capture FIFO writes for test verification
    return n;
}
uint8_t rfm9x_packet_from_fifo(rfm9x_t *r, uint8_t *buf)
{
    // TODO: Allow tests to inject mock received packets
    return 0;
}
void rfm9x_set_tx_irq(rfm9x_t *r, void (*callback)(void))
{
    // TODO: Store callback so tests can trigger TX interrupts
}
void rfm9x_set_rx_irq(rfm9x_t *r, void (*callback)(void))
{
    // TODO: Store callback so tests can trigger RX interrupts
}
void rfm9x_format_packet(packet_t *pkt, uint8_t dst, uint8_t src, uint8_t flags,
                         uint8_t seq, uint8_t len, uint8_t *data)
{
    pkt->dst = dst;
    pkt->src = src;
    pkt->flags = flags;
    pkt->seq = seq;
    pkt->len = len;
    memcpy(pkt->data, data, len);
}
