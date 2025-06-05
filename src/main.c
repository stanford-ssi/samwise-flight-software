/**
 * @author  Niklas Vainio
 * @date    2024-08-23
 *
 * This file contains the main entry point for the SAMWISE flight code.
 */

#include "flash.h"
#include "init.h"
#include "logger.h"
#include "macros.h"
#include "neopixel.h"
#include "rfm9x.h"
#include "scheduler.h"
#include "slate.h"

/**
 * Main code entry point.
 *
 * This should never return (unless something really bad happens!)
 */
int main()
{
    /*
     * Brief delay after reboot/powering up due to power spikes to prevent
     * deployment when satellite is still within the launch mechanism.
     */
    sleep_ms(5000);

    /*
     * Initialize persistent data or load existing data if already in flash.
     * The reboot counter is incremented each time this code runs.
     */
    persistent_data_t *data = init_persistent_data();
    increment_reboot_counter();
    LOG_INFO("Current reboot count: %d\n", data->reboot_counter);

/*
 * Initialize everything.
 */
#ifndef PICO
    // Ensure that PICO_RP2350A is defined to 0 for PICUBED builds.
    // You'll have to overwrite this in your local pico-sdk directory.
    // samwise-flight-software/pico-sdk/src/boards/include/boards/pico2.h
    assert(PICO_RP2350A == 0);
#endif
    LOG_DEBUG("main: Slate uses %d bytes", sizeof(slate));
    LOG_INFO("main: Initializing...");
    ASSERT(init(&slate));
    slate.reboot_counter = data->reboot_counter;
    LOG_INFO("main: Initialized successfully!\n\n\n");

    /*
     * Print commit hash
     */
#ifdef COMMIT_HASH
    LOG_INFO("main: Running samwise-flight-software %s", COMMIT_HASH);
#endif

    neopixel_set_color_rgb(0, 0xff, 0xff);

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
