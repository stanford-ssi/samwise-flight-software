/**
 * OTA hardware test - drives the real flight ota_task_dispatch() on an RP2350.
 *
 * Two variants are built from this file, distinguished by blink rate so the
 * running partition is visible without a serial console:
 *
 *   A (slow blink, 800 ms) - the "known safe" version. Calls the real
 *       ota_task_dispatch(), which reads variant B's image and writes it into
 *       partition B, then reboots into it. The image is served by
 *       filesys_stub.c straight from a byte array rather than by littlefs --
 *       see that file for why.
 *
 *   B (fast blink, 100 ms) - the "new" version, built with
 *       PICO_CRT0_IMAGE_TYPE_TBYB=1 and deliberately never petting the
 *       watchdog, so its TBYB probation lapses and the bootrom rolls back
 *       to A. (Flight code pets it to stay resident; see
 *       ota_mvp/TBYB_TECHNICAL_REFERENCE.md section 6.)
 *
 * Expected sequence on the board:
 *   slow blink  ->  (OTA runs)  ->  fast blink  ->  (TBYB lapses)  ->  slow
 *
 * A has no guard against re-running the OTA, so this cycles indefinitely.
 * That is intentional: the LED sequence is the readout, and each loop is a
 * full demonstration of all three OTA goals -- A is never written and stays
 * bootable, B is written while A runs, and switching works in both
 * directions including automatic rollback.
 *
 * Verified on an RP2350 dev board: 29,424 bytes landed in partition B
 * byte-for-byte, the board booted it, and the bootrom rolled back to A on
 * its own.
 */

#include "pico/bootrom.h"
#include "pico/stdlib.h"
#include <stdio.h>

#ifndef PICO_DEFAULT_LED_PIN
#define PICO_DEFAULT_LED_PIN 25
#endif

#ifdef HW_TEST_VARIANT_B

// ---------------------------------------------------------------- variant B
#define BLINK_MS 100

int main(void)
{
    stdio_init_all();
    const uint led = PICO_DEFAULT_LED_PIN;
    gpio_init(led);
    gpio_set_dir(led, GPIO_OUT);

    while (true)
    {
        // Never call rom_explicit_buy(): the TBYB watchdog must expire and
        // roll the board back to partition A on its own.
        printf("[B] new firmware running (fast blink, TBYB not bought)\n");
        gpio_put(led, 1);
        sleep_ms(BLINK_MS);
        gpio_put(led, 0);
        sleep_ms(BLINK_MS);
    }
}

#else

// ---------------------------------------------------------------- variant A
#include "filesys.h"
#include "ota_task.h"
#include "slate.h"
#include "watchdog.h"

#define BLINK_MS 800

// slate is the global instance declared in slate.h / defined in slate.c

/*
 * There is no serial console on a bare board, so progress and failures are
 * reported on the LED instead:
 *
 *   slow steady blink (800 ms)  - idle / success, OTA already applied
 *   N rapid pulses, repeating   - stopped at stage N:
 *       1 slate init            4 write chunk
 *       2 filesystem init       5 complete write
 *       3 start file write      6 ota_task_dispatch returned (OTA failed)
 */
static uint g_led;

static void pulse(int n)
{
    for (int i = 0; i < n; i++)
    {
        gpio_put(g_led, 1);
        sleep_ms(120);
        gpio_put(g_led, 0);
        sleep_ms(180);
    }
}

static void blink_forever(int code)
{
    while (true)
    {
        pulse(code);
        sleep_ms(1500);
    }
}

/*
 * Announce a stage BEFORE attempting it. If the board goes dark, the last
 * group of pulses seen is the stage it hung inside -- which an error-only
 * scheme cannot distinguish from a hang.
 */
static void stage(int n)
{
    sleep_ms(700);
    pulse(n);
    sleep_ms(700);
}

int main(void)
{
    stdio_init_all();
    const uint led = PICO_DEFAULT_LED_PIN;
    g_led = led;
    gpio_init(led);
    gpio_set_dir(led, GPIO_OUT);

    // Slow blink for a few seconds so the "safe" version is clearly visible
    // before anything happens.
    for (int i = 0; i < 4; i++)
    {
        gpio_put(led, 1);
        sleep_ms(BLINK_MS);
        gpio_put(led, 0);
        sleep_ms(BLINK_MS);
    }

    /*
     * NOTE: do not probe partition B by reading XIP_BASE + <B offset>.
     *
     * The RP2350 remaps the XIP window to the *running* partition, which is
     * why both A and B images are linked at 0x10000000. From inside A, an
     * address past A's own window (152 KiB) is out of bounds and faults --
     * which is exactly how an earlier version of this test died silently
     * before reaching stage 1.
     *
     * Without a guard, A re-runs the OTA on every boot, so the board will
     * cycle A -> B -> A. That is fine here: the LED sequence is the readout,
     * and picotool is not needed while it runs.
     */

    stage(1);
    if (clear_and_init_slate(&slate) != 0)
    {
        blink_forever(1);
    }
    watchdog_init(&slate.watchdog);
    queue_init(&slate.tx_queue, sizeof(packet_t), 8);

    /*
     * No filesystem setup: filesys_stub.c serves the embedded image directly.
     * The real littlefs flash backend faults when running from a partition
     * (see filesys_stub.c), so it is deliberately not in this test.
     */
    stage(2);

    // --- hand off to the REAL flight OTA task ---
    stage(6);
    slate.ota_target_fname[0] = 'b';
    slate.ota_target_fname[1] = '\0';
    slate.ota_requested = true;

    ota_task_dispatch(&slate);

    // Only reached if the OTA failed; rom_reboot does not return on success.
    blink_forever(6);
}

#endif
