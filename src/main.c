/**
 * @author  Niklas Vainio
 * @date    2024-08-23
 *
 * This file contains the main entry point for the SAMWISE flight code.
 */

#include "init.h"
#include "macros.h"

#ifdef TEST
#include "../mock_pico/stdlib.h" // Use mock version for local testing
#else
#include "pico/stdlib.h"
#endif

#include "scheduler/scheduler.h"
#include "slate.h"
#include "testbase.h"

/**
 * Statically allocate the slate.
 */
slate_t slate;

/**
 * Main code entry point.
 *
 * This should never return (unless something really bad happens!)
 */
int main()
{
    /*
     * If we are in test mode, run the tests and exit.
     */
    if (IS_TEST)
    {
        printf("TEST MODE\n");
        return 0; //test_main();
    }
    stdio_init_all();

    /*
     * In debug builds, delay to allow the user to connect to open the serial
     * port.
     */
    if (!IS_FLIGHT)
    {
        sleep_ms(5000);
    }

    /*
     * Initialize everything.
     */
    LOG_INFO("main: Slate uses %d bytes", sizeof(slate));
    LOG_INFO("main: Initializing everything...");
    ASSERT(init(&slate));
    LOG_INFO("main: Initialized successfully!\n\n\n");

    /*
     * Go state machine!
     */
    LOG_INFO("main: Dispatching the state machine...");
    while (true)
    {
        sched_dispatch(&slate);
    }

    /*
     * We should NEVER be here so something bad has happened.
     * @todo reboot!
     */
    ERROR("We reached the end of the code - this is REALLY BAD!");
}
