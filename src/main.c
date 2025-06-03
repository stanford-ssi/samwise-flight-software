/**
 * @author  Niklas Vainio
 * @date    2024-08-23
 *
 * This file contains the main entry point for the SAMWISE flight code.
 */

#include "adm1176.h"
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
     * In debug builds, delay to allow the user to connect to open the serial
     * port.
     */
    if (!IS_FLIGHT)
    {
        sleep_ms(5000);
    }

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

    LOG_INFO("ADM1176: Running ADM1176 test..");

    LOG_INFO("ADM1176: Initializing pwm...");
    adm1176_t pwm;
    adm1176_init(&pwm, SAMWISE_POWER_MONITOR_I2C, ADM1176_I2C_ADDR,
                 ADM1176_DEFAULT_SENSE_RESISTOR);

    LOG_INFO("ADM1176: Turning on pwm");
    adm1176_on(&pwm);

    LOG_INFO("ADM1176: Reading voltage and current...");

    int count = 0;
    while (1)
    {
        float current = adm1176_read_current(&pwm);
        float emf = adm1176_read_voltage(&pwm);

        printf("%d Current: %f, Voltage: %f\n", count, current, emf);
        count++;
        sleep_ms(1000);
    }

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
