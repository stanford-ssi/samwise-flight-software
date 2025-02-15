/**
 * @author  Niklas Vainio, Joseph Shetaye
 * @date    2024-08-27
 *
 * This file should be used to define functions that run when the satellite
 * first boots. This should be used for initializing hardware, setting initial
 * slate values, etc.
 */

#include "init.h"

/**
 * Initialize all gpio pins to their default states.
 *
 * @return True on success, false otherwise.
 */
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

#ifdef BRINGUP

#endif

    return true;
}

static bool init_drivers(slate_t *slate)
{
    slate->onboard_led = onboard_led_mk();
    onboard_led_init(&slate->onboard_led);

    slate->radio = rfm9x_mk();
#ifdef BRINGUP
    gpio_init(SAMWISE_RF_RST_PIN);
    gpio_set_dir(SAMWISE_RF_RST_PIN, GPIO_OUT);
    gpio_put(SAMWISE_RF_RST_PIN, 0);

    gpio_init(SAMWISE_RF_MISO_PIN);
    gpio_set_dir(SAMWISE_RF_MISO_PIN, GPIO_OUT);
    gpio_put(SAMWISE_RF_MISO_PIN, 0);

    gpio_init(SAMWISE_RF_MOSI_PIN);
    gpio_set_dir(SAMWISE_RF_MOSI_PIN, GPIO_OUT);
    gpio_put(SAMWISE_RF_MOSI_PIN, 0);

    gpio_init(SAMWISE_RF_CS_PIN);
    gpio_set_dir(SAMWISE_RF_CS_PIN, GPIO_OUT);
    gpio_put(SAMWISE_RF_CS_PIN, 0);

    gpio_init(SAMWISE_RF_SCK_PIN);
    gpio_set_dir(SAMWISE_RF_SCK_PIN, GPIO_OUT);
    gpio_put(SAMWISE_RF_SCK_PIN, 0);
#else
    rfm9x_init(&slate->radio);
#endif

    return true;
}

/**
 * Primary function called by main to initialize everything.
 *
 * @param slate     Pointer to the (uninitialized) slate
 * @return True on success, false otherwise
 */
bool init(slate_t *slate)
{
    /*
     * Initialize gpio pins
     */
    ASSERT(init_gpio_pins());

    ASSERT(init_drivers(slate));

    /*
     * Initialize the state machine
     */
    sched_init(slate);

    return true;
}
