#ifndef MOCK_PICO_STDLIB_H
#define MOCK_PICO_STDLIB_H
#define GPIO_OUT 1
#define PICO_DEFAULT_LED_PIN 1
#include <stdint.h> 
#include <stdio.h>
#include <stdbool.h>

typedef struct stdio_driver stdio_driver_t;

#define STDIO_ERROR -1
#define STDIO_NO_INPUT -2

static inline void stdio_usb_init() {}
void stdio_uart_init();
static inline void stdio_init_all() { stdio_uart_init(); }
static inline void stdio_filter_driver(stdio_driver_t *driver) {}
static inline void stdio_set_translate_crlf(stdio_driver_t *driver, bool enabled) {}
static inline bool stdio_usb_connected(void) { return true; }
int getchar_timeout_us(uint32_t timeout_us);
#define puts_raw puts
#define putchar_raw putchar

void gpio_init(int gpio);
void gpio_set_dir(int gpio, int out);
void gpio_put(int gpio, int value);
int gpio_get(int gpio);
void sleep_ms(int ms);
void uart_init(int uart, int baud);


#endif
