#include "rfm9x.h"

void rfm9x_print_parameters(rfm9x_t *r)
{
}
uint32_t rfm9x_version(rfm9x_t *r)
{
    return 0x12;
}
uint8_t rfm9x_tx_done(rfm9x_t *r)
{
    return 1;
}
void rfm9x_transmit(rfm9x_t *r)
{
}
void rfm9x_listen(rfm9x_t *r)
{
}
void rfm9x_clear_interrupts(rfm9x_t *r)
{
}
uint8_t rfm9x_packet_to_fifo(rfm9x_t *r, uint8_t *buf, uint8_t n)
{
    return n;
}
uint8_t rfm9x_packet_from_fifo(rfm9x_t *r, uint8_t *buf)
{
    return 0;
}
void rfm9x_set_tx_irq(rfm9x_t *r, void (*callback)(void))
{
}
void rfm9x_set_rx_irq(rfm9x_t *r, void (*callback)(void))
{
}
void rfm9x_format_packet(packet_t *pkt, uint8_t dst, uint8_t src, uint8_t flags,
                         uint8_t seq, uint8_t len, uint8_t *data)
{
}
