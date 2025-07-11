/**
 * @author  Niklas Vainio
 * @date    2024-08-25
 *
 * This file defines several utility functions for error handling.
 *
 * We follow the general rule: "fail loudly on the ground, don't fail in
 * flight". What this means in practice is that non-flight builds should
 * completely fail if an error occurs, whereas flight builds should log the
 * error but not take any drastic action.
 */

#include "error.h"
#include "safe_sleep.h"

/**
 * This function should be called if we encounter an unrecoverable error. In
 * non-flight builds, enter a panic state.
 *
 * @todo In flight, decide on appropriate reboot behavior
 *
 * Note: We do not use the pico's built-in panic function to allow for
 * customized behavior.
 */
void fatal_error(char *msg)
{
#ifdef FLIGHT
    return;
#else
    while (1)
    {
        for (uint32_t i = 0; i < 3; i++)
        {
#ifdef PICO
            gpio_put(PICO_DEFAULT_LED_PIN, 1);
            safe_sleep_ms(100);
            gpio_put(PICO_DEFAULT_LED_PIN, 0);
            safe_sleep_ms(100);
#else
            neopixel_set_color_rgb(0xff, 0x33, 0);
            safe_sleep_ms(100);
            neopixel_set_color_rgb(0, 0, 0);
            safe_sleep_ms(100);
#endif
        }
        printf("ERROR: %s", msg);
        safe_sleep_ms(500);
    }
#endif
}
