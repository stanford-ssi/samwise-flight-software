#include "onboard_led.h"

struct onboard_led onboard_led_mk()
{
    struct onboard_led mock = {0};
    return mock;
}
void onboard_led_init(struct onboard_led *led)
{
    // TODO: Track init state for test assertions
}
void onboard_led_set(struct onboard_led *led, bool val)
{
    // TODO: Track LED state so tests can verify blink patterns
}
bool onboard_led_get(struct onboard_led *led)
{
    // TODO: Return tracked state once set() is implemented
    return false;
}
void onboard_led_toggle(struct onboard_led *led)
{
    // TODO: Toggle tracked state once set()/get() are implemented
}
