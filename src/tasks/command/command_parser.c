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
#include "macros.h"
#include "payload_uart.h"
#include "rfm9x.h"
#include "states.h"
#include <stdio.h>

extern sched_state_t *overridden_state;

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
        case PING:
        {
            LOG_INFO("Retrieving number of commands executed...");
            uint8_t data[PACKET_DATA_SIZE];

            // Package interger value into a string
            int len =
                snprintf(data, sizeof(data), "Number commands executed: %d",
                         slate->number_commands_processed);

            // Create the packet
            packet_t pkt;
            rfm9x_format_packet(&pkt, 0, 0, 0, 0, len, &data[0]);

            // Add to transmit buffer
            LOG_INFO("Sending to radio transmit queue...");
            if (queue_try_add(&slate->tx_queue, &pkt))
            {
                LOG_INFO("Ping info was sent...");
            }
            else
            {
                LOG_ERROR("Ping info failed to send...");
            }
            break;
        }
        case PAYLOAD_TURN_ON:
        {
            LOG_INFO("Turning on payload...");
            payload_turn_on(slate);
            break;
        }

        case PAYLOAD_TURN_OFF:
        {
            LOG_INFO("Turning off payload...");
            payload_turn_off(slate);
            break;
        }
        /* Toggle Commands */
        // TODO: Add more device commands here as needed
        case MANUAL_STATE_OVERRIDE:
        {
            LOG_INFO("Manual state override command received: %s",
                     command_payload);
            if (strcmp(command_payload, "running_state") == 0)
            {
                slate->manual_override_state = &running_state;
            }
            else if (strcmp(command_payload, "init_state") == 0)
            {
                slate->manual_override_state = &init_state;
            }
            else if (strcmp(command_payload, "burn_wire_state") == 0)
            {
                slate->manual_override_state = &burn_wire_state;
            }
            else if (strcmp(command_payload, "burn_wire_reset_state") == 0)
            {
                slate->manual_override_state = &burn_wire_reset_state;
            }
            else
            {
                slate->manual_override_state = NULL;
                LOG_ERROR("Unknown state override command: %s",
                          command_payload);
            }
            break;
        }
        default:
            LOG_ERROR("Unknown command ID: %i", command_id);
            break;
    }
}
