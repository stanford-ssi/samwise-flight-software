#pragma once

#ifdef TEST_MODE
#include <stdint.h>
#include <stdbool.h>
#define PICO_DEFAULT_LED_PIN 25  // Mock LED pin for tests
#else
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#endif

struct onboard_led
{
    unsigned int pin;
    bool on;
};

struct onboard_led onboard_led_mk();
void onboard_led_init(struct onboard_led *led);
void onboard_led_set(struct onboard_led *led, bool val);
bool onboard_led_get(struct onboard_led *led);
void onboard_led_toggle(struct onboard_led *led);
