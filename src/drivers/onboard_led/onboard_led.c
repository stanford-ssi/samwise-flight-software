#include "onboard_led.h"
#include "hal_interface.h"

struct onboard_led onboard_led_mk()
{
    return (struct onboard_led){
        .pin = PICO_DEFAULT_LED_PIN,
        .on = false,
    };
}

void onboard_led_init(struct onboard_led *led)
{
    hal.gpio_init(led->pin);
    hal.gpio_set_dir(led->pin, HAL_GPIO_OUT);
}
void onboard_led_set(struct onboard_led *led, bool val)
{
    led->on = val;
    hal.gpio_put(led->pin, led->on);
}
bool onboard_led_get(struct onboard_led *led)
{
    return led->on;
}
void onboard_led_toggle(struct onboard_led *led)
{
    led->on = !led->on;
    hal.gpio_put(led->pin, led->on);
}
