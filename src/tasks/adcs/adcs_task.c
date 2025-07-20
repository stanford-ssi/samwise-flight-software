/**
 * @author Niklas Vainio
 * @date 2025-05-27
 *
 * ADCS task for high-level ADCS control and command logic
 */

#include "adcs_task.h"
#include "adcs_driver.h"
#include "slate.h"

#define ADCS_HEALTH_CHECK_TIMEOUT_MS (60000)

static absolute_time_t last_telemetry_time;

void adcs_task_init(slate_t *slate)
{
    adcs_driver_init(slate);
    last_telemetry_time = get_absolute_time();
}

void adcs_task_dispatch(slate_t *slate)
{
    // Check if ADCS is alive - reset if no telemetry for too long
    absolute_time_t current_time = get_absolute_time();

    if (adcs_driver_telemetry_available(slate))
    {
        last_telemetry_time = current_time;

        // TODO: Pack the ADCS telemetry into main telemetry
        // adcs_packet_t telem;
        // if (adcs_driver_get_telemetry(slate, &telem) == ADCS_SUCCESS) {
        //     // Process telemetry data for main telemetry packet
        // }
    }
    // Maybe also check if the device is on in the first place.
    // See device status driver: src/drivers/device_status/device_status.c
    //   All that does is read some GPIO pins.
    else if (absolute_time_diff_us(last_telemetry_time, current_time) >
             (ADCS_HEALTH_CHECK_TIMEOUT_MS * 1000))
    {
        // No telemetry for too long - reset ADCS driver
        adcs_driver_reset(slate);
        last_telemetry_time = current_time;
    }
}

sched_task_t adcs_task = {.name = "adcs",
                          // We can dispatch more frequently than telemetry is available
                          // and certainly more frequently than 60s timeout.
                          .dispatch_period_ms = 5000,
                          .task_init = &adcs_task_init,
                          .task_dispatch = &adcs_task_dispatch,

                          /* Set to an actual value on init */
                          .next_dispatch = 0};