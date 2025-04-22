#include "rfm9x.h"
#include "rfm9x_utils.h"
#include <stdio.h>

void rfm9x_print_packet(uint8_t *p, uint8_t l)
{
    printf("Packet of %d bytes: ", l);
    for (int i = 0; i < l; i++)
    {
        printf("0x%02x ", p[i]);
    }
    printf("\\r\\n");
}

void rfm9x_listen(rfm9x_t *r) {
    rfm9x_set_mode(r, RX_MODE);
}

void rfm9x_transmit(rfm9x_t *r) {
    rfm9x_set_mode(r, TX_MODE);
}

