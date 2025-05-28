/**
 * @author  Thomas Haile
 * @date    2025-05-24
 *
 * Command parsing implementation
 */

#include "command_parser.h"
#include "macros.h"

/// @brief Parse packet and dispatch command to appropriate queue
void dispatch_command(slate_t *slate, packet_t *packet)
{
    slate->number_commands_processed++;

    Command command_id = (Command)packet->data[0];
    LOG_INFO("Command ID Received: %i", command_id);

    switch (command_id)
    {
        case NO_OP:
        {
            LOG_INFO("Number of Commands Executed: %d",
                     slate->number_commands_processed);
            break;
        }

        case PAYLOAD_EXEC:
        {
            PAYLOAD_COMMAND_DATA task;
            strlcpy(task.serialized_command, (char *)&packet->data[1],
                    sizeof(task.serialized_command));

            queue_try_add(&slate->payload_command_data, &task);

            LOG_INFO("Payload: %s", task.serialized_command);
            break;
        }

            /* Toggle Commands */
            // TODO: ADD HERE

        default:
            LOG_ERROR("Unknown command ID: %i", command_id);
            break;
    }
}