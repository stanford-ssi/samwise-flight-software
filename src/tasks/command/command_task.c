/**
 * @author  Thomas Haile
 * @date    2025-05-24
 *
 * Task management for command processing
 */

#include "command_task.h"
#include "command_parser.h"
#include "macros.h"
#include "neopixel.h"
#include "pico/stdlib.h"
#include "slate.h"

const int PAYLOAD_DATA_CAPACITY = 32;

// Should be somewhat bigger than FTP_NUM_PACKETS_PER_CYCLE to handle bursts
// of incoming file packets.
const int FTP_WRITE_TO_FILE_DATA_CAPACITY = 16;

/// @brief Initialize the command switch task
/// @param slate Slate
void command_task_init(slate_t *slate)
{
    // Initialize queues for storing processed commands
    queue_init(&slate->payload_command_data, sizeof(PAYLOAD_COMMAND_DATA),
               PAYLOAD_DATA_CAPACITY);

    queue_init(&slate->ftp_start_file_write_data,
               sizeof(FTP_START_FILE_WRITE_DATA),
               1); // We currently only buffer
                   // one file at a time
    queue_init(&slate->ftp_write_to_file_data, sizeof(FTP_WRITE_TO_FILE_DATA),
               FTP_WRITE_TO_FILE_DATA_CAPACITY);
    queue_init(&slate->ftp_cancel_file_write_data,
               sizeof(FTP_CANCEL_FILE_WRITE_DATA),
               1); // Only one cancel command at a time (rejected automatically
                   // if filename doesn't match)

    slate->num_uploaded_bytes = 0;
    slate->packet_buffer_index = 0;
    slate->uploading_command_id = 0;
    slate->number_commands_processed = 0;
}

/// @brief Process incoming radio packets and dispatch commands
void command_task_dispatch(slate_t *slate)
{
    neopixel_set_color_rgb(COMMAND_TASK_COLOR);
    packet_t packet = {0};

    // Process one packet per dispatch cycle
    if (queue_try_remove(&slate->rx_queue, &packet))
    {
        if (!is_packet_authenticated(&packet, slate->reboot_counter))
        {
            LOG_ERROR("Packet authentication failed. Dropping packet.");
            return;
        }

        // Parse and process the command
        dispatch_command(slate, &packet);
    }
    neopixel_set_color_rgb(0, 0, 0);
}

sched_task_t command_task = {.name = "command",
                             .dispatch_period_ms = 100,
                             .task_init = &command_task_init,
                             .task_dispatch = &command_task_dispatch,
                             .next_dispatch = 0};
