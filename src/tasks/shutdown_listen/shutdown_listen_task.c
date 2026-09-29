/**
 * @author  Luis C
 * @date    2026-08-12
 *
 * Receive-only reactivation listener for the shutdown state. See the header
 * for the blackout requirements this task must uphold.
 */

#include "shutdown_listen_task.h"
#include "command_parser.h"
#include "flash.h"
#include "logger.h"
#include "neopixel.h"
#include "packet.h"
#include "radio_task.h"
#include "rfm9x.h"

void shutdown_listen_task_init(slate_t *slate)
{
    // Reuse the radio task's receive pipeline: this installs the RX interrupt
    // handler and initializes rx_queue so packets are decoded and enqueued for
    // us. It does NOT start any transmission (the FIFO is empty, and we never
    // run radio_task_dispatch, which is the only thing that drains tx_queue).
    // This is important on the reboot-into-shutdown path, where the running
    // state (and therefore radio_task_init) never ran.
    radio_task_init(slate);

    // Force receive-only mode and start the reactivation streak fresh.
    rfm9x_listen(&slate->radio);
    slate->shutdown_reactivate_counter = 0;
}

void shutdown_listen_task_dispatch(slate_t *slate)
{
    neopixel_set_color_rgb(SHUTDOWN_LISTEN_TASK_COLOR);

    // Stay in receive mode. We NEVER transmit while in shutdown.
    rfm9x_listen(&slate->radio);

    // Inspect at most one packet per cycle, mirroring command_task.
    packet_t packet = {0};
    if (!queue_try_remove(&slate->rx_queue, &packet))
    {
        neopixel_set_color_rgb(0, 0, 0);
        return;
    }

    if (!is_packet_authenticated(&packet, slate->reboot_counter))
    {
        // Unauthenticated traffic (noise, spoofing) is ignored entirely and
        // does not affect the reactivation streak.
        LOG_INFO("Shutdown: dropped unauthenticated packet");
        neopixel_set_color_rgb(0, 0, 0);
        return;
    }

    Command command_id = (Command)packet.data[0];
    if (command_id == REACTIVATE)
    {
        slate->shutdown_reactivate_counter++;
        LOG_INFO("REACTIVATE command received (%d/%d)",
                 slate->shutdown_reactivate_counter, REACTIVATE_CMD_THRESHOLD);

        if (slate->shutdown_reactivate_counter >= REACTIVATE_CMD_THRESHOLD)
        {
            LOG_INFO("Ground reactivation authorized. Leaving shutdown.");
            clear_shutdown_active();
            slate->shutdown_triggered = false;
            slate->shutdown_cmd_counter = 0;
            slate->shutdown_reactivate_counter = 0;
        }
    }
    else
    {
        // Any other authenticated command breaks the consecutive streak. We
        // take NO action on it, preserving the communication blackout.
        LOG_INFO("Shutdown: ignoring command %i, resetting reactivate streak",
                 command_id);
        slate->shutdown_reactivate_counter = 0;
    }

    neopixel_set_color_rgb(0, 0, 0);
}

sched_task_t shutdown_listen_task = {.name = "shutdown_listen",
                                     .dispatch_period_ms = 1000,
                                     .task_init = &shutdown_listen_task_init,
                                     .task_dispatch =
                                         &shutdown_listen_task_dispatch,
                                     .next_dispatch = 0};
