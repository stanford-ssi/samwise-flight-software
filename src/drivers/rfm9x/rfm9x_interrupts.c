#include "rfm9x.h"
#include "rfm9x_spi.h"
#include "rfm9x_registers.h"

volatile rfm9x_t *radio_with_interrupts = NULL;

/*
 * IRQ Handlers
 */

void rfm9x_interrupt_received()
{
    if (radio_with_interrupts == NULL)
        return;

    uint8_t flags = rfm9x_get_irq_flags(radio_with_interrupts);

    if (flags & 0x40)
    {
        if (radio_with_interrupts->tx_irq != NULL)
            radio_with_interrupts->tx_irq();
    }

    if (flags & 0x40)
    {
        if (radio_with_interrupts->rx_irq != NULL)
            radio_with_interrupts->rx_irq();
    }
    rfm9x_clear_interrupts(radio_with_interrupts, flags);
}

void rfm9x_set_rx_irq(rfm9x_t *r, rfm9x_rx_irq irq) { r->rx_irq = irq; }
void rfm9x_set_tx_irq(rfm9x_t *r, rfm9x_rx_irq irq) { r->tx_irq = irq; }

void rfm9x_clear_interrupts(rfm9x_t *r)
{
    rfm9x_put8(r, _RH_RF95_REG_12_IRQ_FLAGS, 0xff);
}

uint8_t rfm9x_tx_done(rfm9x_t *r) { return (rfm9x_get_irq_flags(r) & 0x08) >> 3; }

uint8_t rfm9x_rx_done(rfm9x_t *r)
{
    uint8_t flags = rfm9x_get_irq_flags(r);
    return (flags & 0x40) >> 6;
}

