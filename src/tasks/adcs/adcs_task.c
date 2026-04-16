/**
 * @author Niklas Vainio
 * @date 2025-05-27
 *
 * ADCS task for high-level ADCS control and command logic
 */

#include "adcs_task.h"
#include "adcs_driver.h"
#include "neopixel.h"
#include "pico/stdlib.h"
#include "slate.h"
#include <string.h>

void test_ack_bytes(slate_t *slate);

#define ADCS_MAX_FAILED_CHECKS_BEFORE_REBOOT (5)

void adcs_task_init(slate_t *slate)
{
    adcs_driver_init(slate);

    slate->adcs_num_failed_checks = 0;

    adcs_driver_power_on(slate);
}

void adcs_task_dispatch(slate_t *slate)
{
    test_ack_bytes(slate);
    neopixel_set_color_rgb(ADCS_TASK_COLOR);

    // Check if the board is alive
    /*if (!adcs_driver_is_alive(slate))
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
    else
    {
        // Board is alive - get telemetry
        adcs_driver_get_telemetry(slate, &slate->adcs_telemetry);
    }*/

    neopixel_set_color_rgb(0, 0, 0);
}

/*
=======
Picubed to ADCS Communication Tests!!!
======
*/

// 1. test that we are receiving acknowledgements. (check if alive, send
// different packets.)

// 2. print out the packet we

void test_ack_bytes(slate_t *slate)
{
    // first, health check packet.
    adcs_command_packet health_packet;
    memset(&health_packet, 0, sizeof(health_packet));
    health_packet.command = HEALTH_CHECK;
    health_packet.packet_length = 0;

    adcs_result_t res = write_adcs_tx_packet(slate, &health_packet);
    if (res == ADCS_WRITE_SUCCESS)
    {
        LOG_DEBUG("[TEST_ACK_BYTES] Write successful! \n");
    }
    else if (res == ADCS_WRITE_CORRUPTED)
    {
        LOG_DEBUG("[TEST_ACK_BYTES] Uh oh, write unsuccessful! \n");
    }
    else
    {
        LOG_DEBUG("[TEST_ACK_BYTES] What is even happening... \n");
    }
}

#ifdef TEST
void send_crappy_packets(slate_t *slate)
{
    adcs_command_packet crappy_packet;
    memset(&crappy_packet, 0, sizeof(crappy_packet));
    crappy_packet.command =
}
#endif

sched_task_t adcs_task = {.name = "adcs",
                          .dispatch_period_ms = 5000,
                          .task_init = &adcs_task_init,
                          .task_dispatch = &adcs_task_dispatch,

                          /* Set to an actual value on init */
                          .next_dispatch = 0};