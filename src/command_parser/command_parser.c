/**
 * @author  Thomas Haile
 * @date    2025-05-24
 *
 * Command parsing implementation.
 * Commands are received as raw packets from rfm9x radio, the command parser
 * decodes the command ID and payload, and dispatches the command to the
 * appropriate queue for downstream processing.
 */

#include "command_parser.h"

/// @brief Parse packet and dispatch command to appropriate queue
void dispatch_command(slate_t *slate, packet_t *packet)
{
    slate->number_commands_processed++;

    Command command_id = (Command)packet->data[0];
    char *command_payload = packet->data + COMMAND_MNEMONIC_SIZE;
    LOG_INFO("Command ID Received: %i", command_id);

    switch (command_id)
    {
        /* Payload Commands */
        case PAYLOAD_EXEC:
        {
            PAYLOAD_COMMAND_DATA payload_command;
            strlcpy(payload_command.serialized_command, command_payload,
                    sizeof(command_payload));
            payload_command.seq_num = slate->curr_command_seq_num++;
            payload_command.command_type = PAYLOAD_EXEC;

            // Add command into queue.
            queue_try_add(&slate->payload_command_data, &payload_command);
            LOG_INFO("Payload: %s", payload_command.serialized_command);
            break;
        }
        case NO_OP:
        {
            LOG_INFO("Number of Commands Executed: %d",
                     slate->number_commands_processed);
            break;
        }
        case PAYLOAD_TURN_ON:
        {
            LOG_INFO("Turning on payload...");
            // Add command into queue.
            queue_try_add(&slate->payload_command_data, &payload_command);
            payload_command.command_type = PAYLOAD_EXEC;
            break;
        }

        case PAYLOAD_TURN_OFF:
        {
            LOG_INFO("Turning off payload...");
            slate->turn_payload_on = false;
            break;
        }
        /* Toggle Commands */
        // TODO: Add more device commands here as needed
        default:
            LOG_ERROR("Unknown command ID: %i", command_id);
            break;
    }
}
