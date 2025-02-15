/**
 * @author Niklas Vainio and Marc Reyes
 * @date 2025-01-18
 */
#include "slate.h"

bool payload_uart_init();

uint16_t payload_uart_read_packet(slate_t *slate, uint8_t *packet);
bool payload_uart_write_packet(slate_t *slate, const uint8_t *packet,
                               uint16_t len, uint16_t seq_num);

void payload_turn_on(slate_t *slate);
void payload_turn_off(slate_t *slate);
