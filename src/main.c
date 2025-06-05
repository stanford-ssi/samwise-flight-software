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
#include "safe_sleep.h"
#include "scheduler.h"
#include "slate.h"

// Slate is initialized in slate.c
extern slate_t slate;

#ifndef PICO
// Ensure that PICO_RP2350A is defined to 0 for PICUBED builds.
// This is to enable full 48pin GPIO support on the RP2350A chip.
// You'll have to overwrite this in your local pico-sdk directory.
// samwise-flight-software/pico-sdk/src/boards/include/boards/pico2.h
static_assert(PICO_RP2350A == 0,
              "PICO_RP2350A must be defined to 0 for PICUBED builds.");
#endif
/**
 * Main code entry point.
 *
 * This should never return (unless something really bad happens!)
 */
int main()
{
    // We need to first initialize watchdog before any sleep is called.
    // Watchdog needs to be fed periodically to prevent rebooting.
    slate.watchdog = watchdog_mk();
    watchdog_init(&slate.watchdog);

/*
 * Brief delay after reboot/powering up due to power spikes to prevent
 * deployment when satellite is still within the launch mechanism.
 */
#ifdef FLIGHT
    // 15 minutes delay is required for dispersion after release from
    // exolaunch deployment chute.
    safe_sleep_ms(15 * 60 * 1000); // 15 minutes
#else
    safe_sleep_ms(5000); // 5 second for debugging
#endif

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
