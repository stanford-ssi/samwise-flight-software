#pragma once

#include "hardware/gpio.h"
#include "pico/stdlib.h"

struct onboard_led
{
    uint pin;
    bool on;
};

struct onboard_led onboard_led_mk();
void onboard_led_init(struct onboard_led *led);
void onboard_led_set(struct onboard_led *led, bool val);
bool onboard_led_get(struct onboard_led *led);
void onboard_led_toggle(struct onboard_led *led);
