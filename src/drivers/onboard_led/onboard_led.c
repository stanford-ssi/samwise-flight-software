#include "onboard_led.h"

struct onboard_led mk_onboard_led() {
  return (struct onboard_led) {
    .pin = PICO_DEFAULT_LED_PIN,
    .on = false,
  };
}

void onboard_led_init(struct onboard_led* led) {
  gpio_init(led->pin);
  gpio_set_dir(led->pin, GPIO_OUT);
}
void onboard_led_set(struct onboard_led* led, bool val) {
  led->on = val;
  gpio_put(led->pin, led->on);
}
bool onboard_led_get(struct onboard_led* led) {
  return led->on;
}
void onboard_led_toggle(struct onboard_led* led) {
  led->on = !led->on;
  gpio_put(led->pin, led->on);
}

