/**
 * @author  Yao Yiheng
 * @date    2025-10-28
 *
 * Minimal implementation of syscalls necessary to boot a different partition
 * for OTA.
 */

#include "pico/stdlib.h"

#ifndef PICO
// Ensure that PICO_RP2350A is defined to 0 for PICUBED builds.
// This is to enable full 48pin GPIO support on the RP2350A chip.
// boards/samwise_picubed.h should define it to 0.
// The CMakeLists.txt file points to this file for the board definition.
static_assert(PICO_RP2350A == 0,
              "PICO_RP2350A must be defined to 0 for PICUBED builds.");
#endif

/**
 * Main code entry point.
 *
 * This should never return (unless something really bad happens!)
 */
int main()
{
    stdio_init_all();

// Initialize LED pin
#ifndef PICO_DEFAULT_LED_PIN
#define PICO_DEFAULT_LED_PIN 25
#endif

    const uint LED_PIN = PICO_DEFAULT_LED_PIN;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 1); // Turn on LED

    // Infinite loop
    while (1)
    {
        printf("OTA MVP Main Running...\n");
        sleep_ms(1000);
    }
}
