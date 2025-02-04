#include "mock_pico.h"

void gpio_put(uint pin, int value) {
    printf("<gpio_put> PIN: %d | Val: %d\n", pin, value);
}

void sleep_ms(uint32_t duration) {
    printf("<sleep_ms> Sleep for: %u\n", duration);
}
