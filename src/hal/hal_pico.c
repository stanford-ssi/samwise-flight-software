/**
 * @file hal_pico.c
 * @author Claude Code
 * @date 2025-01-10
 * 
 * Hardware Abstraction Layer implementation for Raspberry Pi Pico.
 * 
 * This implementation provides direct pass-through to Pico SDK functions,
 * ensuring zero performance overhead for embedded builds.
 */

#include "hal_interface.h"

#ifndef TEST_MODE
// Only include Pico SDK headers when not in test mode
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "hardware/uart.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "pico/time.h"

// Global HAL instance
hal_interface_t hal;

// GPIO wrapper functions
static void hal_pico_gpio_init(hal_pin_t pin) {
    gpio_init(pin);
}

static void hal_pico_gpio_set_dir(hal_pin_t pin, hal_gpio_direction_t direction) {
    gpio_set_dir(pin, direction == HAL_GPIO_OUT);
}

static void hal_pico_gpio_put(hal_pin_t pin, bool value) {
    gpio_put(pin, value);
}

static bool hal_pico_gpio_get(hal_pin_t pin) {
    return gpio_get(pin);
}

static void hal_pico_gpio_set_function(hal_pin_t pin, hal_gpio_function_t function) {
    gpio_set_function(pin, function);
}

static void hal_pico_gpio_pull_up(hal_pin_t pin) {
    gpio_pull_up(pin);
}

static void hal_pico_gpio_pull_down(hal_pin_t pin) {
    gpio_pull_down(pin);
}

static void hal_pico_gpio_disable_pulls(hal_pin_t pin) {
    gpio_disable_pulls(pin);
}

static void hal_pico_gpio_set_irq_enabled_with_callback(hal_pin_t pin, uint32_t events, bool enabled, hal_irq_callback_t callback) {
    gpio_set_irq_enabled_with_callback(pin, events, enabled, (gpio_irq_callback_t)callback);
}

// SPI wrapper functions
static unsigned int hal_pico_spi_init(hal_spi_t spi, uint32_t baudrate) {
    return spi_init((spi_inst_t*)spi, baudrate);
}

static void hal_pico_spi_set_format(hal_spi_t spi, uint8_t data_bits, uint8_t cpol, uint8_t cpha, uint8_t order) {
    spi_set_format((spi_inst_t*)spi, data_bits, (spi_cpol_t)cpol, (spi_cpha_t)cpha, (spi_order_t)order);
}

static int hal_pico_spi_write_blocking(hal_spi_t spi, const uint8_t *src, size_t len) {
    return spi_write_blocking((spi_inst_t*)spi, src, len);
}

static int hal_pico_spi_read_blocking(hal_spi_t spi, uint8_t repeated_tx_data, uint8_t *dst, size_t len) {
    return spi_read_blocking((spi_inst_t*)spi, repeated_tx_data, dst, len);
}

static int hal_pico_spi_write_read_blocking(hal_spi_t spi, const uint8_t *src, uint8_t *dst, size_t len) {
    return spi_write_read_blocking((spi_inst_t*)spi, src, dst, len);
}

// I2C wrapper functions
static unsigned int hal_pico_i2c_init(hal_i2c_t i2c, uint32_t baudrate) {
    return i2c_init((i2c_inst_t*)i2c, baudrate);
}

static int hal_pico_i2c_write_blocking_until(hal_i2c_t i2c, uint8_t addr, const uint8_t *src, size_t len, bool nostop, uint64_t until) {
    absolute_time_t until_time = from_us_since_boot(until);
    return i2c_write_blocking_until((i2c_inst_t*)i2c, addr, src, len, nostop, until_time);
}

static int hal_pico_i2c_read_blocking_until(hal_i2c_t i2c, uint8_t addr, uint8_t *dst, size_t len, bool nostop, uint64_t until) {
    absolute_time_t until_time = from_us_since_boot(until);
    return i2c_read_blocking_until((i2c_inst_t*)i2c, addr, dst, len, nostop, until_time);
}

// UART wrapper functions
static unsigned int hal_pico_uart_init(hal_uart_t uart, uint32_t baudrate) {
    return uart_init((uart_inst_t*)uart, baudrate);
}

static void hal_pico_uart_set_format(hal_uart_t uart, uint8_t data_bits, uint8_t stop_bits, uint8_t parity) {
    uart_set_format((uart_inst_t*)uart, data_bits, stop_bits, (uart_parity_t)parity);
}

static void hal_pico_uart_set_hw_flow(hal_uart_t uart, bool cts, bool rts) {
    uart_set_hw_flow((uart_inst_t*)uart, cts, rts);
}

static void hal_pico_uart_set_fifo_enabled(hal_uart_t uart, bool enabled) {
    uart_set_fifo_enabled((uart_inst_t*)uart, enabled);
}

static void hal_pico_uart_putc_raw(hal_uart_t uart, char c) {
    uart_putc_raw((uart_inst_t*)uart, c);
}

static int hal_pico_uart_getc(hal_uart_t uart) {
    return uart_getc((uart_inst_t*)uart);
}

static bool hal_pico_uart_is_readable(hal_uart_t uart) {
    return uart_is_readable((uart_inst_t*)uart);
}

static bool hal_pico_uart_is_writable(hal_uart_t uart) {
    return uart_is_writable((uart_inst_t*)uart);
}

// PWM wrapper functions
static unsigned int hal_pico_pwm_gpio_to_slice_num(hal_pin_t pin) {
    return pwm_gpio_to_slice_num(pin);
}

static unsigned int hal_pico_pwm_gpio_to_channel(hal_pin_t pin) {
    return pwm_gpio_to_channel(pin);
}

static void hal_pico_pwm_set_wrap(unsigned int slice_num, uint16_t wrap) {
    pwm_set_wrap(slice_num, wrap);
}

static void hal_pico_pwm_set_chan_level(unsigned int slice_num, unsigned int chan, uint16_t level) {
    pwm_set_chan_level(slice_num, chan, level);
}

static void hal_pico_pwm_set_enabled(unsigned int slice_num, bool enabled) {
    pwm_set_enabled(slice_num, enabled);
}

// Timing wrapper functions
static void hal_pico_sleep_ms(uint32_t ms) {
    sleep_ms(ms);
}

static void hal_pico_sleep_us(uint64_t us) {
    sleep_us(us);
}

static void hal_pico_busy_wait_ms(uint32_t ms) {
    busy_wait_ms(ms);
}

static void hal_pico_busy_wait_us(uint64_t us) {
    busy_wait_us(us);
}

static uint64_t hal_pico_get_absolute_time_us(void) {
    return to_us_since_boot(get_absolute_time());
}

static uint32_t hal_pico_time_us_32(void) {
    return time_us_32();
}

static uint64_t hal_pico_make_timeout_time_ms(uint32_t ms) {
    return to_us_since_boot(make_timeout_time_ms(ms));
}

static int64_t hal_pico_absolute_time_diff_us(uint64_t from, uint64_t to) {
    absolute_time_t from_time = from_us_since_boot(from);
    absolute_time_t to_time = from_us_since_boot(to);
    return absolute_time_diff_us(from_time, to_time);
}

void hal_init(void) {
    // Initialize GPIO function pointers
    hal.gpio_init = hal_pico_gpio_init;
    hal.gpio_set_dir = hal_pico_gpio_set_dir;
    hal.gpio_put = hal_pico_gpio_put;
    hal.gpio_get = hal_pico_gpio_get;
    hal.gpio_set_function = hal_pico_gpio_set_function;
    hal.gpio_pull_up = hal_pico_gpio_pull_up;
    hal.gpio_pull_down = hal_pico_gpio_pull_down;
    hal.gpio_disable_pulls = hal_pico_gpio_disable_pulls;
    hal.gpio_set_irq_enabled_with_callback = hal_pico_gpio_set_irq_enabled_with_callback;
    
    // Initialize SPI function pointers
    hal.spi_init = hal_pico_spi_init;
    hal.spi_set_format = hal_pico_spi_set_format;
    hal.spi_write_blocking = hal_pico_spi_write_blocking;
    hal.spi_read_blocking = hal_pico_spi_read_blocking;
    hal.spi_write_read_blocking = hal_pico_spi_write_read_blocking;
    
    // Initialize I2C function pointers
    hal.i2c_init = hal_pico_i2c_init;
    hal.i2c_write_blocking_until = hal_pico_i2c_write_blocking_until;
    hal.i2c_read_blocking_until = hal_pico_i2c_read_blocking_until;
    
    // Initialize UART function pointers
    hal.uart_init = hal_pico_uart_init;
    hal.uart_set_format = hal_pico_uart_set_format;
    hal.uart_set_hw_flow = hal_pico_uart_set_hw_flow;
    hal.uart_set_fifo_enabled = hal_pico_uart_set_fifo_enabled;
    hal.uart_putc_raw = hal_pico_uart_putc_raw;
    hal.uart_getc = hal_pico_uart_getc;
    hal.uart_is_readable = hal_pico_uart_is_readable;
    hal.uart_is_writable = hal_pico_uart_is_writable;
    
    // Initialize PWM function pointers
    hal.pwm_gpio_to_slice_num = hal_pico_pwm_gpio_to_slice_num;
    hal.pwm_gpio_to_channel = hal_pico_pwm_gpio_to_channel;
    hal.pwm_set_wrap = hal_pico_pwm_set_wrap;
    hal.pwm_set_chan_level = hal_pico_pwm_set_chan_level;
    hal.pwm_set_enabled = hal_pico_pwm_set_enabled;
    
    // Initialize timing function pointers
    hal.sleep_ms = hal_pico_sleep_ms;
    hal.sleep_us = hal_pico_sleep_us;
    hal.busy_wait_ms = hal_pico_busy_wait_ms;
    hal.busy_wait_us = hal_pico_busy_wait_us;
    hal.get_absolute_time_us = hal_pico_get_absolute_time_us;
    hal.time_us_32 = hal_pico_time_us_32;
    hal.make_timeout_time_ms = hal_pico_make_timeout_time_ms;
    hal.absolute_time_diff_us = hal_pico_absolute_time_diff_us;
}

#endif // !TEST_MODE