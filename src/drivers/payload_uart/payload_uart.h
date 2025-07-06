/**
 * @author Niklas Vainio and Marc Reyes
 * @date 2025-01-18
 */
#pragma once
#include "slate.h"

typedef enum
{
    SUCCESSFUL_WRITE = 0,
    PACKET_TOO_BIG,
    SYN_UNSUCCESSFUL,
    UART_WRITE_TIMEDOUT,
    HEADER_UNACKNOWLEDGED,
    FINAL_WRITE_UNSUCCESSFUL
} payload_write_error_code;

bool payload_uart_init();

uint16_t payload_uart_read_packet(slate_t *slate, uint8_t *packet);
payload_write_error_code payload_uart_write_packet(slate_t *slate,
                                                   const uint8_t *packet,
                                                   uint16_t len,
                                                   uint16_t seq_num);

void payload_turn_on(slate_t *slate);
void payload_turn_off(slate_t *slate);
