/**
 * @author Niklas Vainio
 * @date 2025-05-27
 *
 * ADCS task for high-level ADCS control and command logic
 */

#include "adcs_task.h"
#include "adcs_driver.h"
#include "slate.h"

#define ADCS_MAX_FAILED_CHECKS_BEFORE_REBOOT (5)

void adcs_task_init(slate_t *slate)
{
    adcs_driver_init(slate);

    slate->adcs_num_failed_checks = 0;
}

void adcs_task_dispatch(slate_t *slate)
{
    // Check if the board is alive
    if (!adcs_driver_is_alive(slate))
    {
        // Not alive - increment persistence counter and reboot if persistent
        slate->adcs_num_failed_checks++;

        if (slate->adcs_num_failed_checks >=
            ADCS_MAX_FAILED_CHECKS_BEFORE_REBOOT)
        {
            slate->adcs_num_failed_checks = 0;

            adcs_driver_power_off(slate);
            sleep_ms(100);
            adcs_driver_power_on(slate);
        }
    }

    // Board is alive - get telemetry
    adcs_driver_get_telemetry(slate, &slate->adcs_telemetry);
}

sched_task_t adcs_task = {.name = "adcs",
                          .dispatch_period_ms = 5000,
                          .task_init = &adcs_task_init,
                          .task_dispatch = &adcs_task_dispatch,

                          /* Set to an actual value on init */
                          .next_dispatch = 0};