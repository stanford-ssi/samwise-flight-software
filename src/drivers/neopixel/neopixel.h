/**
 * @author Isaac Lee
 * @date 2025-01-11
 *
 * Sets neopixel states on Picube
 */
#pragma once

#ifndef TEST
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "logger.h"
#include "pico/stdlib.h"
#include "pins.h"
#include "ws2812.pio.h"
#else
#include <stdint.h>
#endif

void neopixel_init();
void neopixel_set_color_rgb(uint8_t r, uint8_t g, uint8_t b);
