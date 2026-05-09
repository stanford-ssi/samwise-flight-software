
#include "neopixel.h"

#if !defined(PICO) || defined(PICOHAT)
// Neopixel available on PICUBED and PICO with radio hat

// Maximum time to wait for the PIO TX FIFO to drain when pushing a pixel.
// If the state machine has stalled this prevents an infinite block.
#define NEOPIXEL_PUT_TIMEOUT_US (10000) // 10 ms

static PIO neo_pio;
static uint neo_sm;

static inline void put_pixel(uint32_t pixel_grb)
{
    uint32_t value = pixel_grb << 8u;
    absolute_time_t start = get_absolute_time();

    // Non-blocking equivalent of pio_sm_put_blocking with a timeout, so we
    // cannot get stuck forever if the PIO state machine stalls.
    while (pio_sm_is_tx_fifo_full(neo_pio, neo_sm))
    {
        if (absolute_time_diff_us(start, get_absolute_time()) >
            NEOPIXEL_PUT_TIMEOUT_US)
        {
            LOG_ERROR("Neopixel PIO TX FIFO full - dropping pixel");
            return;
        }
        tight_loop_contents();
    }
    pio_sm_put(neo_pio, neo_sm, value);
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
