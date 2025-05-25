/**
 * @author  Thomas Haile
 * @date    2025-05-24
 *
 * Task management for command processing
 */

#include "command_task.h"
#include "command_parser.h"
#include "macros.h"
#include "pico/stdlib.h"
#include "slate.h"

#define PACKET_BYTE_LENGTH 251 // in bytes TODO check?/ get from driver mod?

const int RADIO_PACKETS_OUT_MAX_LENGTH = 64;
const int TAKE_PHOTO_QUEUE_CAPACITY = 32;
const int DOWNLOAD_PHOTO_QUEUE_CAPACITY = 32;
const int TAKE_AND_SEND_QUEUE_CAPACITY = 32;

/// @brief Initialize the command switch task
/// @param slate Slate
void command_task_init(slate_t *slate)
{
    // Initialize queue for radio input data
    queue_init(&slate->rx_queue, PACKET_BYTE_LENGTH * sizeof(uint8_t),
               RADIO_PACKETS_OUT_MAX_LENGTH);

    // Initialize queues for storing processed commands
    queue_init(&slate->take_photo_task_data, sizeof(PAYLOAD_COMMAND_DATA),
               TAKE_PHOTO_QUEUE_CAPACITY);
    queue_init(&slate->download_photo_task_data, sizeof(PAYLOAD_COMMAND_DATA),
               DOWNLOAD_PHOTO_QUEUE_CAPACITY);
    queue_init(&slate->take_and_send_photo_task_data,
               sizeof(PAYLOAD_COMMAND_DATA), TAKE_AND_SEND_QUEUE_CAPACITY);

    slate->num_uploaded_bytes = 0;
    slate->packet_buffer_index = 0;
    slate->uploading_command_id = 0;
    slate->number_commands_processed = 0;
}

/// @brief Process incoming radio packets and dispatch commands
void command_task_dispatch(slate_t *slate)
{
    packet_t packet;

    // Process one packet per dispatch cycle
    if (queue_try_remove(&slate->rx_queue, &packet))
    {
        if (!is_packet_authenticated(&packet))
        {
            LOG_ERROR("Packet authentication failed. Dropping packet.");
            return;
        }

        // Parse and process the command
        dispatch_command(slate, &packet);
    }
}

sched_task_t command_task = {.name = "command",
                             .dispatch_period_ms = 100,
                             .task_init = &command_task_init,
                             .task_dispatch = &command_task_dispatch,
                             .next_dispatch = 0};