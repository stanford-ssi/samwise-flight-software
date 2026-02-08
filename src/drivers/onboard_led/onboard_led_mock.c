#include "onboard_led.h"

struct onboard_led onboard_led_mk()
{
    struct onboard_led mock = {0};
    return mock;
}
void onboard_led_init(struct onboard_led *led)
{
}
void onboard_led_set(struct onboard_led *led, bool val)
{
}
bool onboard_led_get(struct onboard_led *led)
{
    return false;
}
void onboard_led_toggle(struct onboard_led *led)
{
}
