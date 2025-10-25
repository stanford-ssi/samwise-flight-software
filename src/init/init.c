#include "init.h"
#include "burn_wire.h"

// GPIO pin definitions for telemetry
#define CHRG_STATUS 34
#define FAULT_STATUS 33
#define SIDE_PANEL_A 10
#define SIDE_PANEL_B 9
#define RBF_DETECT 42

static bool init_gpio_pins()
{
#ifndef PICO
    // Initialize I2C for MPPT
    i2c_init(SAMWISE_MPPT_I2C, 100 * 1000);
    gpio_set_function(SAMWISE_MPPT_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SAMWISE_MPPT_SCL_PIN, GPIO_FUNC_I2C);

    // Initialize I2C for power monitor
    i2c_init(SAMWISE_POWER_MONITOR_I2C, 100 * 1000);
    gpio_set_function(SAMWISE_POWER_MONITOR_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SAMWISE_POWER_MONITOR_SCL_PIN, GPIO_FUNC_I2C);
#endif

    // Initialize telemetry pins for PICUBED
#ifndef PICO
    gpio_init(CHRG_STATUS);
    gpio_set_dir(CHRG_STATUS, GPIO_IN);

    gpio_init(FAULT_STATUS);
    gpio_set_dir(FAULT_STATUS, GPIO_IN);

    gpio_init(SIDE_PANEL_A);
    gpio_set_dir(SIDE_PANEL_A, GPIO_IN);

    gpio_init(SIDE_PANEL_B);
    gpio_set_dir(SIDE_PANEL_B, GPIO_IN);

    gpio_init(RBF_DETECT);
    gpio_set_dir(RBF_DETECT, GPIO_IN);
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

    return true;
}
