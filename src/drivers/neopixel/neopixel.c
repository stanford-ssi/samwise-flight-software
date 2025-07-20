
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
    put_pixel(ugrb_u32(g, r, b), SAMWISE_NEOPIXEL_PIN);
}
#else
void neopixel_init()
{
    LOG_INFO("[neopixel] Fake init neopixel driver - hardware only exists on "
             "PICUBED.");
}

void neopixel_set_color_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    LOG_DEBUG("Setting neopixel on Pin <MISSING> rgb values: %u %u %u", r, g,
              b);
}
#endif