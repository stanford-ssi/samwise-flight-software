#include "payload_uart.h"

bool payload_uart_init(slate_t *slate)
{
    return true;
}
uint16_t payload_uart_read_packet(slate_t *slate, uint8_t *packet)
{
    return 0;
}
payload_write_error_code payload_uart_write_packet(slate_t *slate,
                                                   const uint8_t *packet,
                                                   uint16_t len,
                                                   uint16_t seq_num)
{
    return SUCCESSFUL_WRITE;
}
void payload_restart(slate_t *slate)
{
}
void payload_turn_on(slate_t *slate)
{
}
void payload_turn_off(slate_t *slate)
{
}
