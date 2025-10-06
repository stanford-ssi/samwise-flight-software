
/**
 * @author  Niklas Vainio, Joseph Shetaye
 * @date    2024-08-27
 *
 * This file should be used to define functions that run when the satellite
 * first boots. This should be used for initializing hardware, setting initial
 * slate values, etc.
 */

#include "init.h"

#include "burn_wire.h"
#include "hal_interface.h"

/**
 * Initialize all gpio pins to their default states.
 *
 * @return True on success, false otherwise.
 */
static bool init_gpio_pins()
{
#ifndef PICO
    hal.i2c_init((hal_i2c_t)SAMWISE_MPPT_I2C, 100 * 1000);
    hal.gpio_set_function(SAMWISE_MPPT_SDA_PIN, HAL_GPIO_FUNC_I2C);
    hal.gpio_set_function(SAMWISE_MPPT_SCL_PIN, HAL_GPIO_FUNC_I2C);

    hal.i2c_init((hal_i2c_t)SAMWISE_POWER_MONITOR_I2C, 100 * 1000);
    hal.gpio_set_function(SAMWISE_POWER_MONITOR_SDA_PIN, HAL_GPIO_FUNC_I2C);
    hal.gpio_set_function(SAMWISE_POWER_MONITOR_SCL_PIN, HAL_GPIO_FUNC_I2C);

    hal.gpio_init(SAMWISE_RBF_DETECT_PIN);
    hal.gpio_set_dir(SAMWISE_RBF_DETECT_PIN, HAL_GPIO_IN);
#endif

#ifdef BRINGUP

#endif

    return true;
}

static bool init_drivers(slate_t *slate)
{
    slate->onboard_led = onboard_led_mk();
    onboard_led_init(&slate->onboard_led);

    logger_init();

    slate->radio = rfm9x_mk();
#ifdef BRINGUP
    hal.gpio_init(SAMWISE_RF_RST_PIN);
    hal.gpio_set_dir(SAMWISE_RF_RST_PIN, HAL_GPIO_OUT);
    hal.gpio_put(SAMWISE_RF_RST_PIN, 0);

    hal.gpio_init(SAMWISE_RF_MISO_PIN);
    hal.gpio_set_dir(SAMWISE_RF_MISO_PIN, HAL_GPIO_OUT);
    hal.gpio_put(SAMWISE_RF_MISO_PIN, 0);

    hal.gpio_init(SAMWISE_RF_MOSI_PIN);
    hal.gpio_set_dir(SAMWISE_RF_MOSI_PIN, HAL_GPIO_OUT);
    hal.gpio_put(SAMWISE_RF_MOSI_PIN, 0);

    hal.gpio_init(SAMWISE_RF_CS_PIN);
    hal.gpio_set_dir(SAMWISE_RF_CS_PIN, HAL_GPIO_OUT);
    hal.gpio_put(SAMWISE_RF_CS_PIN, 0);

    hal.gpio_init(SAMWISE_RF_SCK_PIN);
    hal.gpio_set_dir(SAMWISE_RF_SCK_PIN, HAL_GPIO_OUT);
    hal.gpio_put(SAMWISE_RF_SCK_PIN, 0);
#else
    rfm9x_init(&slate->radio);
#endif

    // Initialize Neopixel if on PICUBED
#ifndef PICO
    neopixel_init();
#endif

    // Initialize burn wire
    burn_wire_init(slate);

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

    hal.gpio_init(SAMWISE_WATCHDOG_FEED_PIN);
    hal.gpio_set_dir(SAMWISE_WATCHDOG_FEED_PIN, HAL_GPIO_OUT);
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
