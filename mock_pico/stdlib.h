#ifndef MOCK_PICO_STDLIB_H
#define MOCK_PICO_STDLIB_H

#include "mock_pico/printf.h"
#include <stdint.h>
#include <stdbool.h>
#include "mock_pico/types.h"

// Mock for GPIO pin initialization
void gpio_init(uint pin) {
    printf("Mock: GPIO pin %d initialized\n", pin);
}

// Mock for setting GPIO direction
void gpio_set_dir(uint pin, bool out) {
    printf("Mock: GPIO pin %d set to direction %s\n", pin, out ? "OUTPUT" : "INPUT");
}

// Mock for setting GPIO state
void gpio_put(uint pin, bool value) {
    printf("Mock: GPIO pin %d set to %s\n", pin, value ? "HIGH" : "LOW");
}

// Mock for reading GPIO state
bool gpio_get(uint pin) {
    printf("Mock: GPIO pin %d read\n", pin);
    return true; // Simulate high signal
}

// Mock for sleep/delay function
void sleep_ms(uint32_t ms) {
    printf("Mock: Sleep for %d ms\n", ms);
}

// Mock for UART initialization (if used)
void uart_init(void) {
    printf("Mock: UART initialized\n");
}

// You can add more mocked functions based on what you use in your code

#endif // MOCK_PICO_STDLIB_H
