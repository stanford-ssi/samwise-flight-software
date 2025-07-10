/**
 * @file hal_interface.h
 * @author Claude Code  
 * @date 2025-01-10
 * 
 * Hardware Abstraction Layer (HAL) interface definitions.
 * 
 * This HAL provides a unified interface for hardware operations while maintaining
 * zero performance overhead for embedded builds. For embedded targets, HAL calls
 * are implemented as direct function calls to Pico SDK functions. For test builds,
 * HAL calls are routed through mock implementations.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Forward declarations for hardware types
typedef unsigned int hal_pin_t;
typedef unsigned int hal_spi_t;
typedef unsigned int hal_i2c_t;
typedef unsigned int hal_uart_t;

typedef enum {
    HAL_GPIO_IN = 0,
    HAL_GPIO_OUT = 1
} hal_gpio_direction_t;

typedef enum {
    HAL_GPIO_FUNC_SPI = 1,
    HAL_GPIO_FUNC_UART = 2,
    HAL_GPIO_FUNC_I2C = 3,
    HAL_GPIO_FUNC_PWM = 4,
    HAL_GPIO_FUNC_SIO = 5,
    HAL_GPIO_FUNC_PIO0 = 6,
    HAL_GPIO_FUNC_PIO1 = 7,
    HAL_GPIO_FUNC_GPCK = 8,
    HAL_GPIO_FUNC_USB = 9,
    HAL_GPIO_FUNC_NULL = 0x1f
} hal_gpio_function_t;

typedef enum {
    HAL_GPIO_PULL_NONE = 0,
    HAL_GPIO_PULL_UP = 1,
    HAL_GPIO_PULL_DOWN = 2
} hal_gpio_pull_t;

typedef enum {
    HAL_IRQ_EDGE_FALL = 0x1,
    HAL_IRQ_EDGE_RISE = 0x2,
    HAL_IRQ_EDGE_BOTH = 0x3
} hal_irq_edge_t;

typedef void (*hal_irq_callback_t)(hal_pin_t pin, uint32_t events);

// HAL Interface Structure
typedef struct {
    // GPIO Interface
    void (*gpio_init)(hal_pin_t pin);
    void (*gpio_set_dir)(hal_pin_t pin, hal_gpio_direction_t direction);
    void (*gpio_put)(hal_pin_t pin, bool value);
    bool (*gpio_get)(hal_pin_t pin);
    void (*gpio_set_function)(hal_pin_t pin, hal_gpio_function_t function);
    void (*gpio_pull_up)(hal_pin_t pin);
    void (*gpio_pull_down)(hal_pin_t pin);
    void (*gpio_disable_pulls)(hal_pin_t pin);
    void (*gpio_set_irq_enabled_with_callback)(hal_pin_t pin, uint32_t events, bool enabled, hal_irq_callback_t callback);
    
    // SPI Interface
    unsigned int (*spi_init)(hal_spi_t spi, uint32_t baudrate);
    void (*spi_set_format)(hal_spi_t spi, uint8_t data_bits, uint8_t cpol, uint8_t cpha, uint8_t order);
    int (*spi_write_blocking)(hal_spi_t spi, const uint8_t *src, size_t len);
    int (*spi_read_blocking)(hal_spi_t spi, uint8_t repeated_tx_data, uint8_t *dst, size_t len);
    int (*spi_write_read_blocking)(hal_spi_t spi, const uint8_t *src, uint8_t *dst, size_t len);
    
    // I2C Interface
    unsigned int (*i2c_init)(hal_i2c_t i2c, uint32_t baudrate);
    int (*i2c_write_blocking_until)(hal_i2c_t i2c, uint8_t addr, const uint8_t *src, size_t len, bool nostop, uint64_t until);
    int (*i2c_read_blocking_until)(hal_i2c_t i2c, uint8_t addr, uint8_t *dst, size_t len, bool nostop, uint64_t until);
    
    // UART Interface
    unsigned int (*uart_init)(hal_uart_t uart, uint32_t baudrate);
    void (*uart_set_format)(hal_uart_t uart, uint8_t data_bits, uint8_t stop_bits, uint8_t parity);
    void (*uart_set_hw_flow)(hal_uart_t uart, bool cts, bool rts);
    void (*uart_set_fifo_enabled)(hal_uart_t uart, bool enabled);
    void (*uart_putc_raw)(hal_uart_t uart, char c);
    int (*uart_getc)(hal_uart_t uart);
    bool (*uart_is_readable)(hal_uart_t uart);
    bool (*uart_is_writable)(hal_uart_t uart);
    
    // PWM Interface
    unsigned int (*pwm_gpio_to_slice_num)(hal_pin_t pin);
    unsigned int (*pwm_gpio_to_channel)(hal_pin_t pin);
    void (*pwm_set_wrap)(unsigned int slice_num, uint16_t wrap);
    void (*pwm_set_chan_level)(unsigned int slice_num, unsigned int chan, uint16_t level);
    void (*pwm_set_enabled)(unsigned int slice_num, bool enabled);
    
    // Timing Interface
    void (*sleep_ms)(uint32_t ms);
    void (*sleep_us)(uint64_t us);
    void (*busy_wait_ms)(uint32_t ms);
    void (*busy_wait_us)(uint64_t us);
    uint64_t (*get_absolute_time_us)(void);
    uint32_t (*time_us_32)(void);
    uint64_t (*make_timeout_time_ms)(uint32_t ms);
    int64_t (*absolute_time_diff_us)(uint64_t from, uint64_t to);
    
    // Flash Interface
    void (*flash_range_erase)(uint32_t flash_offs, size_t count);
    void (*flash_range_program)(uint32_t flash_offs, const uint8_t *data, size_t count);
    
    // Interrupt Control Interface
    uint32_t (*save_and_disable_interrupts)(void);
    void (*restore_interrupts)(uint32_t status);
    
} hal_interface_t;

// Global HAL instance
extern hal_interface_t hal;

// HAL initialization functions
void hal_init(void);

#ifdef TEST_MODE
// Test-specific HAL functions
void hal_mock_init(void);
void hal_mock_reset(void);

// Mock test helper functions
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