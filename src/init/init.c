
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
    LOG_DEBUG("init: onboard_led_mk...");
    slate->onboard_led = onboard_led_mk();
    onboard_led_init(&slate->onboard_led);

    LOG_DEBUG("init: logger_init...");
    logger_init();

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
#endif

    // Initialize radio if on PICUBED or PICO with radio hat
#if !defined(PICO) || defined(PICOHAT)
    LOG_DEBUG("init: rfm9x_mk...");
    slate->radio = rfm9x_mk();
    LOG_DEBUG("init: rfm9x_init...");
    rfm9x_init(&slate->radio);
    LOG_DEBUG("init: rfm9x_init done");
#endif

    // Initialize neopixel if on PICUBED or PICO with radio hat
#if !defined(PICO) || defined(PICOHAT)
    LOG_DEBUG("init: neopixel_init...");
    neopixel_init();
#endif

    // Initialize remaining drivers if on PICUBED
#ifndef PICO

    LOG_DEBUG("init: burn_wire_init...");
    // Initialize burn wire
    burn_wire_init(slate);
#endif

    LOG_DEBUG("init: init_drivers done");
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

    gpio_init(SAMWISE_WATCHDOG_FEED_PIN);
    gpio_set_dir(SAMWISE_WATCHDOG_FEED_PIN, GPIO_OUT);
    /*
     * Initialize gpio pins
     */
    LOG_DEBUG("init: init_gpio_pins...");
    ASSERT(init_gpio_pins());

    LOG_DEBUG("init: init_drivers...");
    ASSERT(init_drivers(slate));

    /*
     * Initialize the state machine
     */
    LOG_DEBUG("init: sched_init...");
    sched_init(slate);

    return true;
}
