/**
 * @author  Niklas Vainio
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
#ifdef PICO
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
#else
    // Default i2c
    i2c_init(SAMWISE_MPPT_I2C, 100 * 1000);
    gpio_set_function(SAMWISE_MPPT_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SAMWISE_MPPT_SCL_PIN, GPIO_FUNC_I2C);

    i2c_init(SAMWISE_POWER_MONITOR_I2C, 100 * 1000);
    gpio_set_function(SAMWISE_POWER_MONITOR_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SAMWISE_POWER_MONITOR_SCL_PIN, GPIO_FUNC_I2C);

    gpio_init(SAMWISE_RF_REGULATOR_PIN);
    gpio_set_dir(SAMWISE_RF_REGULATOR_PIN, GPIO_OUT);
#endif

#ifdef BRINGUP
    // RF pins are pulled low in bringup
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

    /*
     * Initialize the state machine
     */
    sched_init(slate);

    return true;
}
