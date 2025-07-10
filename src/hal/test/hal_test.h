/**
 * @file hal_test.h
 * @author Claude Code
 * @date 2025-01-10
 * 
 * Header file for HAL tests.
 */

#pragma once

#include "hal_interface.h"

#ifdef TEST_MODE
// Mock test helper functions
void hal_mock_reset(void);
void hal_mock_set_pin_value(hal_pin_t pin, bool value);
bool hal_mock_get_pin_value(hal_pin_t pin);
void hal_mock_advance_time(uint32_t ms);
uint32_t hal_mock_get_spi_transaction_count(void);
uint32_t hal_mock_get_i2c_transaction_count(void);
uint32_t hal_mock_get_uart_transaction_count(void);
void hal_mock_set_spi_read_data(const uint8_t *data, size_t len);
void hal_mock_set_i2c_read_data(const uint8_t *data, size_t len);
void hal_mock_fail_next_spi(void);
void hal_mock_fail_next_i2c(void);
void hal_mock_fail_next_uart(void);
#endif