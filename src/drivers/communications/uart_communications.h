#pragma once
/*
 * Author:      @Carson Lauer
 * Date:        April 26, 2026
 * Description: Uart communication library.
 * This library includes a irq_callback to tx uart messages from a buffer
 * and to receive uart messages into a buffer. It assumes either COBS
 * encoded packets or null-terminated strings (both end in an 0x00 char).
 */

#include "hardware/uart.h"
#include <stdint.h>

uint16_t uart_comms_rx_count(uart_inst_t *uart_instance);

/**
 * Initialize UART communication on the specified instance
 *
 * @param uart_instance UART peripheral instance number
 * @param tx TX pin number
 * @param rx RX pin number
 * @param baud Baud rate for communication
 */
void uart_comms_init(uart_inst_t *uart_instance, uint8_t tx, uint8_t rx,
                     uint32_t baud);

/**
 * Transmit a message via UART
 *
 * @param uart_instance UART peripheral instance number
 * @param data Pointer to data buffer to transmit
 * @param length Length of data to transmit
 * @return Number of bytes queued for transmission
 */
uint16_t uart_comms_tx(uart_inst_t *uart_instance, uint8_t *data,
                       uint16_t length);

/**
 * Check if a complete packet has been received
 *
 * @param uart_instance UART peripheral instance number
 * @return Number of bytes in the next complete packet, or 0 if no packet ready
 */
uint16_t uart_comms_packet_ready(uart_inst_t *uart_instance);

/**
 * Retrieve a received packet from the buffer
 *
 * @param uart_instance UART peripheral instance number
 * @param buffer Pointer to buffer to store received data
 * @param max_length Maximum length of data to retrieve
 * @return Number of bytes copied to buffer
 */
uint16_t uart_comms_get_packet(uart_inst_t *uart_instance, uint8_t *buffer,
                               uint16_t max_length);

/**
 * UART interrupt handler callback
 *
 * @param uart_instance UART peripheral instance number
 */
void uart0_irq_handler(void);
void uart1_irq_handler(void);
