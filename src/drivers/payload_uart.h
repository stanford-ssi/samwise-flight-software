/**
 * @author Niklas Vainio and Marc Reyes
 * @date 2025-01-18
 */

bool payload_uart_init();

uint16_t payload_uart_read_packet(uint8_t *packet);
bool payload_uart_write_packet(const uint8_t *packet, uint16_t len,
                               uint16_t seq_num)
