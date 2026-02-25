
#include "neopixel.h"

#ifndef PICO
// Neopixel only available on PICUBED boards
static inline void put_pixel(uint32_t pixel_grb, uint pin)
{
    pio_sm_put_blocking(pio0, pin, pixel_grb << 8);
}

static inline uint32_t ugrb_u32(uint8_t g, uint8_t r, uint8_t b)
{
    return ((uint32_t)(g) << 16) | ((uint32_t)(r) << 8) | (uint32_t)(b);
}

void neopixel_init()
{
    PIO pio = pio0;
    uint sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);

    ws2812_program_init(pio, sm, offset, SAMWISE_NEOPIXEL_PIN, 800000.0f, true);
}

void neopixel_set_color_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    // LOG_DEBUG("Setting neopixel on Pin %u rgb values: %u %u %u",
    //           SAMWISE_NEOPIXEL_PIN, r, g, b);
    // Remaps 0-255 to 0-31 for 5-bit color depth
    r = r >> 3;
    g = g >> 3;
    b = b >> 3;
    put_pixel(ugrb_u32(g, r, b), SAMWISE_NEOPIXEL_PIN);
}
#else
// PICO fallback - use onboard LED
void neopixel_init()
{
    // Onboard LED is already initialized in init_drivers()
    LOG_INFO("[neopixel] Using onboard LED fallback for PICO platform.");
}

void neopixel_set_color_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    // Turn LED on if any color channel is non-zero, off otherwise
    bool led_state = (r > 0 || g > 0 || b > 0);
    gpio_put(PICO_DEFAULT_LED_PIN, led_state);
}
#endif
