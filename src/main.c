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
// boards/samwise_picubed.h should define it to 0.
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
    if (clear_and_init_slate(&slate) != 0)
    {
        ERROR("main: Failed to initialize slate! THIS IS REALLY BAD! "
              "Continuing with half-initialized slate, but expect more errors "
              "down the line...");
        // We won't exit, as most likely only filesys failed. We will just keep
        // running without a slate, but this is really bad and will likely cause
        // more errors down the line.
    }

#ifndef TEST
    // Initialize USB stdio early for PICO platform so logging works
    stdio_usb_init();
#endif

    sleep_ms(1000); // Sleep for a bit to let the USB connection establish for
                    // logging
    LOG_DEBUG("main: Starting main function...");

#ifndef PICO
    // We need to first initialize watchdog before any sleep is called.
    // Watchdog needs to be fed periodically to prevent rebooting.
    slate.watchdog = watchdog_mk();
    watchdog_init(&slate.watchdog);

    LOG_DEBUG("main: Watchdog initialized");
#endif

    // Initialize hardware status GPIO pins.
    // This is used to read the status of the solar panels, RBF, etc.
    // Primarily at this point in time, we need to verify the RBF status.
    device_status_init();

    LOG_DEBUG("main: Device initialization complete, entering main loop...");

/*
 * Brief delay after reboot/powering up due to power spikes to prevent
 * deployment when satellite is still within the launch mechanism.
 */
#ifdef FLIGHT
    // 15 minutes delay is required for dispersion after release from
    // exolaunch deployment chute.
    safe_sleep_ms(15 * 60 * 1000); // 15 minutes
#else
    LOG_DEBUG("main: Sleeping 5s...");
    safe_sleep_ms(5000); // 5 second for debugging
    LOG_DEBUG("main: Sleep done");
#endif

    /*
     * Initialize persistent data or load existing data if already in flash.
     * The reboot counter is incremented each time this code runs.
     */
    LOG_DEBUG("main: Initializing persistent data...");
    persistent_data_t *data = init_persistent_data();
    LOG_DEBUG("main: Persistent data initialized, reboot count = %d",
              data->reboot_counter);
    increment_reboot_counter();
    LOG_DEBUG("      rebot_counter++ -> %d", data->reboot_counter);

    /*
     * Initialize everything.
     */
    LOG_DEBUG("main: Slate uses %d bytes", sizeof(slate));
    LOG_INFO("main: Initializing...");
    LOG_DEBUG("main: Calling init()...");
    ASSERT(init(&slate));
    slate.reboot_counter = data->reboot_counter;

    LOG_INFO("main: Starting SAMWISE flight software...");
    LOG_INFO("Current reboot count: %d\n", data->reboot_counter);

#ifdef PACKET_HMAC_PSK
    LOG_INFO("main: HMAC_PSK <ENABLED>");
#else
    LOG_INFO("main: HMAC_PSK <OFF>");
#endif

    LOG_INFO("main: Initialized successfully!\n\n\n");

    /*
     * Print commit hash
     */
#ifdef COMMIT_HASH
    LOG_INFO("main: Running samwise-flight-software %s", COMMIT_HASH);
#endif

    // At this point, all initialization is done.
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
