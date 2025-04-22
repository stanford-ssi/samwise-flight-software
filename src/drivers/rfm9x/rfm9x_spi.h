#ifndef RFM9X_SPI_H_
#define RFM9X_SPI_H_

#include "rfm9x.h"

/*
 * Read a buffer from a register address.
 */
void rfm9x_get_buf(rfm9x_t *r, rfm9x_reg_t reg, uint8_t *buf, uint32_t n);

/*
 * Write a buffer to a register address.
 */
void rfm9x_put_buf(rfm9x_t *r, rfm9x_reg_t reg, uint8_t *buf, uint32_t n);

/*
 * Write a single byte to an RFM9X register
 */
void rfm9x_put8(rfm9x_t *r, rfm9x_reg_t reg, uint8_t v);

/*
 * Get a single byte from an RFM9X register
 */
uint8_t rfm9x_get8(rfm9x_t *r, rfm9x_reg_t reg);

void rfm9x_reset(rfm9x_t *r);

#endif /* RFM9X_SPI_H_ */

