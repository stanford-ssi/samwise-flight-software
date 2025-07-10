/**
 * @file hal_mock.c
 * @author Claude Code
 * @date 2025-01-10
 * 
 * Mock Hardware Abstraction Layer implementation for testing.
 * 
 * This implementation provides controllable mock hardware for unit and
 * integration testing without requiring physical hardware.
 */

#include "hal_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Forward declarations for test helper functions
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

#ifdef TEST_MODE

// Mock hardware state
typedef struct {
    bool pin_states[32];           // GPIO pin states
    bool pin_initialized[32];      // GPIO pin init status
    hal_gpio_direction_t pin_directions[32];
    hal_gpio_function_t pin_functions[32];
    hal_gpio_pull_t pin_pulls[32];
    
    uint64_t current_time_us;      // Mock time counter
    uint32_t spi_transactions;     // Count of SPI operations
    uint32_t i2c_transactions;     // Count of I2C operations
    uint32_t uart_transactions;    // Count of UART operations
    
    bool fail_next_spi;            // Simulate SPI failure
    bool fail_next_i2c;            // Simulate I2C failure
    bool fail_next_uart;           // Simulate UART failure
    
    uint8_t spi_read_data[256];    // Mock SPI read data
    uint8_t i2c_read_data[256];    // Mock I2C read data
    size_t spi_read_index;
    size_t i2c_read_index;
    
} hal_mock_state_t;

static hal_mock_state_t mock_state;

// Global HAL instance
hal_interface_t hal;

// GPIO mock functions
static void hal_mock_gpio_init(hal_pin_t pin) {
    assert(pin < 32);
    mock_state.pin_initialized[pin] = true;
    mock_state.pin_states[pin] = false;
    mock_state.pin_directions[pin] = HAL_GPIO_IN;
    mock_state.pin_functions[pin] = HAL_GPIO_FUNC_SIO;
    mock_state.pin_pulls[pin] = HAL_GPIO_PULL_NONE;
}

static void hal_mock_gpio_set_dir(hal_pin_t pin, hal_gpio_direction_t direction) {
    assert(pin < 32);
    assert(mock_state.pin_initialized[pin]);
    mock_state.pin_directions[pin] = direction;
}

static void hal_mock_gpio_put(hal_pin_t pin, bool value) {
    assert(pin < 32);
    assert(mock_state.pin_initialized[pin]);
    assert(mock_state.pin_directions[pin] == HAL_GPIO_OUT);
    mock_state.pin_states[pin] = value;
}

static bool hal_mock_gpio_get(hal_pin_t pin) {
    assert(pin < 32);
    assert(mock_state.pin_initialized[pin]);
    return mock_state.pin_states[pin];
}

static void hal_mock_gpio_set_function(hal_pin_t pin, hal_gpio_function_t function) {
    assert(pin < 32);
    assert(mock_state.pin_initialized[pin]);
    mock_state.pin_functions[pin] = function;
}

static void hal_mock_gpio_pull_up(hal_pin_t pin) {
    assert(pin < 32);
    assert(mock_state.pin_initialized[pin]);
    mock_state.pin_pulls[pin] = HAL_GPIO_PULL_UP;
}

static void hal_mock_gpio_pull_down(hal_pin_t pin) {
    assert(pin < 32);
    assert(mock_state.pin_initialized[pin]);
    mock_state.pin_pulls[pin] = HAL_GPIO_PULL_DOWN;
}

static void hal_mock_gpio_disable_pulls(hal_pin_t pin) {
    assert(pin < 32);
    assert(mock_state.pin_initialized[pin]);
    mock_state.pin_pulls[pin] = HAL_GPIO_PULL_NONE;
}

static void hal_mock_gpio_set_irq_enabled_with_callback(hal_pin_t pin, uint32_t events, bool enabled, hal_irq_callback_t callback) {
    assert(pin < 32);
    assert(mock_state.pin_initialized[pin]);
    // Mock implementation - just store the callback for testing
    (void)events;
    (void)enabled;
    (void)callback;
}

// SPI mock functions
static unsigned int hal_mock_spi_init(hal_spi_t spi, uint32_t baudrate) {
    (void)spi;
    return baudrate; // Return the requested baudrate
}

static void hal_mock_spi_set_format(hal_spi_t spi, uint8_t data_bits, uint8_t cpol, uint8_t cpha, uint8_t order) {
    (void)spi;
    (void)data_bits;
    (void)cpol;
    (void)cpha;
    (void)order;
}

static int hal_mock_spi_write_blocking(hal_spi_t spi, const uint8_t *src, size_t len) {
    (void)spi;
    (void)src;
    
    if (mock_state.fail_next_spi) {
        mock_state.fail_next_spi = false;
        return -1;
    }
    
    mock_state.spi_transactions++;
    return (int)len;
}

static int hal_mock_spi_read_blocking(hal_spi_t spi, uint8_t repeated_tx_data, uint8_t *dst, size_t len) {
    (void)spi;
    (void)repeated_tx_data;
    
    if (mock_state.fail_next_spi) {
        mock_state.fail_next_spi = false;
        return -1;
    }
    
    // Copy mock data to destination
    size_t copy_len = len;
    if (mock_state.spi_read_index + copy_len > sizeof(mock_state.spi_read_data)) {
        copy_len = sizeof(mock_state.spi_read_data) - mock_state.spi_read_index;
    }
    
    memcpy(dst, &mock_state.spi_read_data[mock_state.spi_read_index], copy_len);
    mock_state.spi_read_index += copy_len;
    mock_state.spi_transactions++;
    
    return (int)copy_len;
}

static int hal_mock_spi_write_read_blocking(hal_spi_t spi, const uint8_t *src, uint8_t *dst, size_t len) {
    (void)spi;
    (void)src;
    
    if (mock_state.fail_next_spi) {
        mock_state.fail_next_spi = false;
        return -1;
    }
    
    // Copy mock data to destination
    size_t copy_len = len;
    if (mock_state.spi_read_index + copy_len > sizeof(mock_state.spi_read_data)) {
        copy_len = sizeof(mock_state.spi_read_data) - mock_state.spi_read_index;
    }
    
    memcpy(dst, &mock_state.spi_read_data[mock_state.spi_read_index], copy_len);
    mock_state.spi_read_index += copy_len;
    mock_state.spi_transactions++;
    
    return (int)copy_len;
}

// I2C mock functions
static unsigned int hal_mock_i2c_init(hal_i2c_t i2c, uint32_t baudrate) {
    (void)i2c;
    return baudrate;
}

static int hal_mock_i2c_write_blocking_until(hal_i2c_t i2c, uint8_t addr, const uint8_t *src, size_t len, bool nostop, uint64_t until) {
    (void)i2c;
    (void)addr;
    (void)src;
    (void)nostop;
    (void)until;
    
    if (mock_state.fail_next_i2c) {
        mock_state.fail_next_i2c = false;
        return -1;
    }
    
    mock_state.i2c_transactions++;
    return (int)len;
}

static int hal_mock_i2c_read_blocking_until(hal_i2c_t i2c, uint8_t addr, uint8_t *dst, size_t len, bool nostop, uint64_t until) {
    (void)i2c;
    (void)addr;
    (void)nostop;
    (void)until;
    
    if (mock_state.fail_next_i2c) {
        mock_state.fail_next_i2c = false;
        return -1;
    }
    
    // Copy mock data to destination
    size_t copy_len = len;
    if (mock_state.i2c_read_index + copy_len > sizeof(mock_state.i2c_read_data)) {
        copy_len = sizeof(mock_state.i2c_read_data) - mock_state.i2c_read_index;
    }
    
    memcpy(dst, &mock_state.i2c_read_data[mock_state.i2c_read_index], copy_len);
    mock_state.i2c_read_index += copy_len;
    mock_state.i2c_transactions++;
    
    return (int)copy_len;
}

// UART mock functions
static unsigned int hal_mock_uart_init(hal_uart_t uart, uint32_t baudrate) {
    (void)uart;
    return baudrate;
}

static void hal_mock_uart_set_format(hal_uart_t uart, uint8_t data_bits, uint8_t stop_bits, uint8_t parity) {
    (void)uart;
    (void)data_bits;
    (void)stop_bits;
    (void)parity;
}

static void hal_mock_uart_set_hw_flow(hal_uart_t uart, bool cts, bool rts) {
    (void)uart;
    (void)cts;
    (void)rts;
}

static void hal_mock_uart_set_fifo_enabled(hal_uart_t uart, bool enabled) {
    (void)uart;
    (void)enabled;
}

static void hal_mock_uart_putc_raw(hal_uart_t uart, char c) {
    (void)uart;
    (void)c;
    mock_state.uart_transactions++;
}

static int hal_mock_uart_getc(hal_uart_t uart) {
    (void)uart;
    mock_state.uart_transactions++;
    return -1; // No data available
}

static bool hal_mock_uart_is_readable(hal_uart_t uart) {
    (void)uart;
    return false; // No data available
}

static bool hal_mock_uart_is_writable(hal_uart_t uart) {
    (void)uart;
    return true; // Always writable
}

// PWM mock functions
static unsigned int hal_mock_pwm_gpio_to_slice_num(hal_pin_t pin) {
    return pin / 2; // Simple mapping for testing
}

static unsigned int hal_mock_pwm_gpio_to_channel(hal_pin_t pin) {
    return pin % 2; // Simple mapping for testing
}

static void hal_mock_pwm_set_wrap(unsigned int slice_num, uint16_t wrap) {
    (void)slice_num;
    (void)wrap;
}

static void hal_mock_pwm_set_chan_level(unsigned int slice_num, unsigned int chan, uint16_t level) {
    (void)slice_num;
    (void)chan;
    (void)level;
}

static void hal_mock_pwm_set_enabled(unsigned int slice_num, bool enabled) {
    (void)slice_num;
    (void)enabled;
}

// Timing mock functions
static void hal_mock_sleep_ms(uint32_t ms) {
    mock_state.current_time_us += ms * 1000;
}

static void hal_mock_sleep_us(uint64_t us) {
    mock_state.current_time_us += us;
}

static void hal_mock_busy_wait_ms(uint32_t ms) {
    mock_state.current_time_us += ms * 1000;
}

static void hal_mock_busy_wait_us(uint64_t us) {
    mock_state.current_time_us += us;
}

static uint64_t hal_mock_get_absolute_time_us(void) {
    return mock_state.current_time_us;
}

static uint32_t hal_mock_time_us_32(void) {
    return (uint32_t)mock_state.current_time_us;
}

static uint64_t hal_mock_make_timeout_time_ms(uint32_t ms) {
    return mock_state.current_time_us + (ms * 1000);
}

static int64_t hal_mock_absolute_time_diff_us(uint64_t from, uint64_t to) {
    return (int64_t)(to - from);
}

// Flash mock functions
static void hal_mock_flash_range_erase(uint32_t flash_offs, size_t count) {
    // Mock implementation - just track the operation
    (void)flash_offs;
    (void)count;
    // Could store flash operations for verification in tests
}

static void hal_mock_flash_range_program(uint32_t flash_offs, const uint8_t *data, size_t count) {
    // Mock implementation - just track the operation
    (void)flash_offs;
    (void)data;
    (void)count;
    // Could store flash operations for verification in tests
}

// Interrupt control mock functions
static uint32_t hal_mock_save_and_disable_interrupts(void) {
    // Mock implementation - return a dummy status
    return 0xDEADBEEF;
}

static void hal_mock_restore_interrupts(uint32_t status) {
    // Mock implementation - just verify the status is what we returned
    (void)status;
    assert(status == 0xDEADBEEF);
}

void hal_mock_init(void) {
    // Initialize GPIO function pointers
    hal.gpio_init = hal_mock_gpio_init;
    hal.gpio_set_dir = hal_mock_gpio_set_dir;
    hal.gpio_put = hal_mock_gpio_put;
    hal.gpio_get = hal_mock_gpio_get;
    hal.gpio_set_function = hal_mock_gpio_set_function;
    hal.gpio_pull_up = hal_mock_gpio_pull_up;
    hal.gpio_pull_down = hal_mock_gpio_pull_down;
    hal.gpio_disable_pulls = hal_mock_gpio_disable_pulls;
    hal.gpio_set_irq_enabled_with_callback = hal_mock_gpio_set_irq_enabled_with_callback;
    
    // Initialize SPI function pointers
    hal.spi_init = hal_mock_spi_init;
    hal.spi_set_format = hal_mock_spi_set_format;
    hal.spi_write_blocking = hal_mock_spi_write_blocking;
    hal.spi_read_blocking = hal_mock_spi_read_blocking;
    hal.spi_write_read_blocking = hal_mock_spi_write_read_blocking;
    
    // Initialize I2C function pointers
    hal.i2c_init = hal_mock_i2c_init;
    hal.i2c_write_blocking_until = hal_mock_i2c_write_blocking_until;
    hal.i2c_read_blocking_until = hal_mock_i2c_read_blocking_until;
    
    // Initialize UART function pointers
    hal.uart_init = hal_mock_uart_init;
    hal.uart_set_format = hal_mock_uart_set_format;
    hal.uart_set_hw_flow = hal_mock_uart_set_hw_flow;
    hal.uart_set_fifo_enabled = hal_mock_uart_set_fifo_enabled;
    hal.uart_putc_raw = hal_mock_uart_putc_raw;
    hal.uart_getc = hal_mock_uart_getc;
    hal.uart_is_readable = hal_mock_uart_is_readable;
    hal.uart_is_writable = hal_mock_uart_is_writable;
    
    // Initialize PWM function pointers
    hal.pwm_gpio_to_slice_num = hal_mock_pwm_gpio_to_slice_num;
    hal.pwm_gpio_to_channel = hal_mock_pwm_gpio_to_channel;
    hal.pwm_set_wrap = hal_mock_pwm_set_wrap;
    hal.pwm_set_chan_level = hal_mock_pwm_set_chan_level;
    hal.pwm_set_enabled = hal_mock_pwm_set_enabled;
    
    // Initialize timing function pointers
    hal.sleep_ms = hal_mock_sleep_ms;
    hal.sleep_us = hal_mock_sleep_us;
    hal.busy_wait_ms = hal_mock_busy_wait_ms;
    hal.busy_wait_us = hal_mock_busy_wait_us;
    hal.get_absolute_time_us = hal_mock_get_absolute_time_us;
    hal.time_us_32 = hal_mock_time_us_32;
    hal.make_timeout_time_ms = hal_mock_make_timeout_time_ms;
    hal.absolute_time_diff_us = hal_mock_absolute_time_diff_us;
    
    // Initialize flash function pointers
    hal.flash_range_erase = hal_mock_flash_range_erase;
    hal.flash_range_program = hal_mock_flash_range_program;
    
    // Initialize interrupt control function pointers
    hal.save_and_disable_interrupts = hal_mock_save_and_disable_interrupts;
    hal.restore_interrupts = hal_mock_restore_interrupts;
}

void hal_init(void) {
    hal_mock_init();
}

void hal_mock_reset(void) {
    memset(&mock_state, 0, sizeof(mock_state));
}

// Test helper functions
void hal_mock_set_pin_value(hal_pin_t pin, bool value) {
    assert(pin < 32);
    mock_state.pin_states[pin] = value;
}

bool hal_mock_get_pin_value(hal_pin_t pin) {
    assert(pin < 32);
    return mock_state.pin_states[pin];
}

void hal_mock_advance_time(uint32_t ms) {
    mock_state.current_time_us += ms * 1000;
}

uint32_t hal_mock_get_spi_transaction_count(void) {
    return mock_state.spi_transactions;
}

uint32_t hal_mock_get_i2c_transaction_count(void) {
    return mock_state.i2c_transactions;
}

uint32_t hal_mock_get_uart_transaction_count(void) {
    return mock_state.uart_transactions;
}

void hal_mock_set_spi_read_data(const uint8_t *data, size_t len) {
    assert(len <= sizeof(mock_state.spi_read_data));
    memcpy(mock_state.spi_read_data, data, len);
    mock_state.spi_read_index = 0;
}

void hal_mock_set_i2c_read_data(const uint8_t *data, size_t len) {
    assert(len <= sizeof(mock_state.i2c_read_data));
    memcpy(mock_state.i2c_read_data, data, len);
    mock_state.i2c_read_index = 0;
}

void hal_mock_fail_next_spi(void) {
    mock_state.fail_next_spi = true;
}

void hal_mock_fail_next_i2c(void) {
    mock_state.fail_next_i2c = true;
}

void hal_mock_fail_next_uart(void) {
    mock_state.fail_next_uart = true;
}

#endif // TEST_MODE