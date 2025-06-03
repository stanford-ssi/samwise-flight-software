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
const int PAYLOAD_DATA_CAPACITY = 32;

/// @brief Initialize the command switch task
/// @param slate Slate
void command_task_init(slate_t *slate)
{
    // Initialize queue for radio input data
    queue_init(&slate->rx_queue, PACKET_BYTE_LENGTH * sizeof(uint8_t),
               RADIO_PACKETS_OUT_MAX_LENGTH);

    // Initialize queues for storing processed commands
    queue_init(&slate->payload_command_data, sizeof(PAYLOAD_COMMAND_DATA),
               PAYLOAD_DATA_CAPACITY);

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
        LOG_INFO("Received packet with length: %d", packet.len);
        // rfm9x_print_packet("Command: ", &packet, packet.len + 5);
        if (!is_packet_authenticated(&packet, slate->reboot_counter))
        {
            LOG_ERROR("Packet authentication failed. Dropping packet.");
            return;
        }
        LOG_INFO("Packet authenticated successfully.");

        // Parse and process the command
        dispatch_command(slate, &packet);
    }
}

sched_task_t command_task = {.name = "command",
                             .dispatch_period_ms = 100,
                             .task_init = &command_task_init,
                             .task_dispatch = &command_task_dispatch,
                             .next_dispatch = 0};
