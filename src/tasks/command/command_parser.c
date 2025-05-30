/**
 * @author  Thomas Haile
 * @date    2025-05-24
 *
 * Command parsing implementation
 */

#include "command_parser.h"
#include "macros.h"
#include "payload_uart.h"

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
            PAYLOAD_COMMAND_DATA payload_str;
            strlcpy(payload_str.serialized_command, command_payload,
                    sizeof(command_payload));

            // Add command into queue.
            queue_try_add(&slate->payload_command_data, &payload_str);
            LOG_INFO("Payload: %s", payload_str.serialized_command);

            // TODO: actual execution should be within payload task.
            if (slate->is_payload_on)
            {
                payload_uart_write_packet(slate, payload_str.serialized_command,
                                          sizeof(command_payload),
                                          slate->curr_command_seq_num);
                slate->curr_command_seq_num++;
            }
            else
            {
                // TODO: A way to let ground station know that Payload is turned
                // off
                LOG_DEBUG("Payload is turned off, please turn payload on first "
                          "and then redo the command!");
            }
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
            payload_turn_on(slate);
            break;
        }

        case PAYLOAD_TURN_OFF:
        {
            payload_turn_off(slate);
            break;

            /* Toggle Commands */
            // TODO: ADD HERE
        }

        default:
            LOG_ERROR("Unknown command ID: %i", command_id);
            break;
    }
}
