#include "cobs.h"
#include "logger.h"
#include "protocol.h"
#include "uart_communications.h"

uint16_t uart_comms_rx_count(uart_inst_t *uart_instance)
{
    LOG_INFO("UART MOCK");
    return 0;
}

void uart_comms_init(uart_inst_t *uart_instance, uint8_t tx, uint8_t rx,
                     uint32_t baud)
{
    LOG_INFO("UART MOCK");
}

uint16_t uart_comms_tx(uart_inst_t *uart_instance, uint8_t *data,
                       uint16_t length)
{
    LOG_INFO("UART MOCK");
    return 0;
}

uint16_t uart_comms_packet_ready(uart_inst_t *uart_instance)
{
    LOG_INFO("UART MOCK");
    return 0;
}

uint16_t uart_comms_get_packet(uart_inst_t *uart_instance, uint8_t *buffer,
                               uint16_t max_length)
{
    LOG_INFO("UART MOCK");
    return 0;
}

void uart0_irq_handler(void)
{
    LOG_INFO("UART MOCK");
}
