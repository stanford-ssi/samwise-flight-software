#ifndef RFM9X_INTERRUPTS_H_
#define RFM9X_INTERRUPTS_H_

#include "rfm9x.h"

/*
 * IRQ Handlers
 */
void rfm9x_interrupt_received();
void rfm9x_set_rx_irq(rfm9x_t *r, rfm9x_rx_irq irq);
void rfm9x_set_tx_irq(rfm9x_t *r, rfm9x_rx_irq irq);
void rfm9x_clear_interrupts(rfm9x_t *r);
uint8_t rfm9x_tx_done(rfm9x_t *r);
uint8_t rfm9x_rx_done(rfm9x_t *r);

#endif /* RFM9X_INTERRUPTS_H_ */

