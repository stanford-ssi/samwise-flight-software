#ifndef RFM9X_UTILS_H_
#define RFM9X_UTILS_H_

#include "rfm9x.h"

void rfm9x_print_packet(uint8_t *p, uint8_t l);
void rfm9x_listen(rfm9x_t *r);
void rfm9x_transmit(rfm9x_t *r);

#endif /* RFM9X_UTILS_H_ */

