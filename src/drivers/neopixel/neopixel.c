
#include "neopixel.h"

#if !defined(PICO) || defined(PICOHAT)
// Neopixel available on PICUBED and PICO with radio hat

static PIO neo_pio;
static uint neo_sm;

static inline void put_pixel(uint32_t pixel_grb)
{
    pio_sm_put_blocking(neo_pio, neo_sm, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b)
{
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}

void neopixel_init()
{
    gpio_init(SAMWISE_NEOPIXEL_PIN);
    gpio_set_dir(SAMWISE_NEOPIXEL_PIN, GPIO_OUT);
    gpio_put(SAMWISE_NEOPIXEL_PIN, 0);

    neo_pio = pio0;
    neo_sm = pio_claim_unused_sm(neo_pio, true);
    uint offset = pio_add_program(neo_pio, &ws2812_program);
    ws2812_program_init(neo_pio, neo_sm, offset, SAMWISE_NEOPIXEL_PIN,
                        800000.0f, false);
}

void neopixel_set_color_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    put_pixel(urgb_u32(r >> 3, g >> 3, b >> 3));
}
#else // plain PICO without hat
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
