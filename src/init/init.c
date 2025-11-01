#include "init.h"
#include "burn_wire.h"
#include "telemetry_pins.h"

static bool init_gpio_pins()
{
#ifndef PICO
    i2c_init(SAMWISE_MPPT_I2C, 100 * 1000);
    gpio_set_function(SAMWISE_MPPT_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SAMWISE_MPPT_SCL_PIN, GPIO_FUNC_I2C);

    i2c_init(SAMWISE_POWER_MONITOR_I2C, 100 * 1000);
    gpio_set_function(SAMWISE_POWER_MONITOR_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SAMWISE_POWER_MONITOR_SCL_PIN, GPIO_FUNC_I2C);
#endif

    return true;
}

static bool init_drivers(slate_t *slate)
{
    slate->onboard_led = onboard_led_mk();
    onboard_led_init(&slate->onboard_led);

    logger_init();

    slate->radio = rfm9x_mk();
#ifndef BRINGUP
    rfm9x_init(&slate->radio);
#endif

#ifndef PICO
    neopixel_init();
#endif

    burn_wire_init(slate);

    return true;
}

bool init(slate_t *slate)
{
    gpio_init(SAMWISE_WATCHDOG_FEED_PIN);
    gpio_set_dir(SAMWISE_WATCHDOG_FEED_PIN, GPIO_OUT);

    ASSERT(init_gpio_pins());
    ASSERT(init_drivers(slate));
    sched_init(slate);

    gpio_init(PIN_CHRG);
    gpio_set_dir(PIN_CHRG, GPIO_IN);

    gpio_init(PIN_FAULT);
    gpio_set_dir(PIN_FAULT, GPIO_IN);

    gpio_init(PIN_PANEL_A);
    gpio_set_dir(PIN_PANEL_A, GPIO_IN);

    gpio_init(PIN_PANEL_B);
    gpio_set_dir(PIN_PANEL_B, GPIO_IN);

    gpio_init(PIN_RBF);
    gpio_set_dir(PIN_RBF, GPIO_IN);

    return true;
}