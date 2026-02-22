#include "payload_uart.h"

bool payload_uart_init(slate_t *slate)
{
    // TODO: Track init state for test assertions
    return true;
}
uint16_t payload_uart_read_packet(slate_t *slate, uint8_t *packet)
{
    // TODO: Allow tests to inject mock payload packets
    return 0;
}
payload_write_error_code payload_uart_write_packet(slate_t *slate,
                                                   const uint8_t *packet,
                                                   uint16_t len,
                                                   uint16_t seq_num)
{
    // TODO: Capture written packets for test verification
    return SUCCESSFUL_WRITE;
}
void payload_restart(slate_t *slate)
{
    // TODO: Track restart calls for test assertions
}
void payload_turn_on(slate_t *slate)
{
    // TODO: Track power state for test assertions
}
void payload_turn_off(slate_t *slate)
{
    // TODO: Track power state for test assertions
}
