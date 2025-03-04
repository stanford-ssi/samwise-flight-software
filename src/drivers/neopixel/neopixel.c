#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "pico/stdlib.h"

#include "neopixel.h"
#include "ws2812.pio.h"

static inline void put_pixel(uint32_t pixel_grb, uint pin)
{
    pio_sm_put_blocking(pio0, pin, pixel_grb << 8);
}

static inline uint32_t ugrb_u32(uint8_t g, uint8_t r, uint8_t b)
{
    return ((uint32_t)(g) << 16) | ((uint32_t)(r) << 8) | (uint32_t)(b);
}

void neopixel_init(uint ledPin)
{
    PIO pio = pio0;
    uint sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    // char str[12];

    ws2812_program_init(pio, sm, offset, ledPin, 800000.0f, true);
}

void neopixel_set_color_rgb(uint8_t r, uint8_t g, uint8_t b, uint ledPin)
{
    put_pixel(ugrb_u32(g, r, b), ledPin);
}