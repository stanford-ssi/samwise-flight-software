/**
 * @author  Niklas Vainio
 * @date    2024-08-27
 *
 * This file should be used to define functions that run when the satellite
 * first boots. This should be used for initializing hardware, setting initial
 * slate values, etc.
 */

#include "init.h"
#include "macros.h"
#include "pico/stdlib.h"
#include "pico/i2c.h"
#include "scheduler/scheduler.h"

/**
 * Initialize all gpio pins to their default states.
 *
 * @return True on success, false otherwise.
 */
static bool init_gpio_pins()
{
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    return true;
}

/**
 * Initialize the I2C interface.
 *
 * @return True on success, false otherwise.
 */
static bool init_i2c()
{
    // TODO: Replace SDA, SCL, baud rate
    const uint i2c_sda_pin = 4;
    const uint i2c_scl_pin = 5;
    const uint i2c_baudrate = 100000;  // Standard I2C baud rate (100 kHz)

    // Initialize the I2C hardware
    i2c_init(i2c_default, i2c_baudrate);

    // Configure the GPIO pins for I2C functionality
    gpio_set_function(i2c_sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(i2c_scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(i2c_sda_pin);
    gpio_pull_up(i2c_scl_pin);

    // TODO: add device specific checks

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
