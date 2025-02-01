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
#include "scheduler/scheduler.h"
#include "pico/stdlib.h"

// #ifdef TEST
// #include <stdlib.h>
// #else
// #include "pico/stdlib.h"
// #endif

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
