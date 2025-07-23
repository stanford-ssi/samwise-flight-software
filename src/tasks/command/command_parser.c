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
#include "states.h"
#include <stdlib.h>

extern sched_state_t *overridden_state;

char *isolate_argument(char *payload_data, char *res_string,
                       size_t *isolated_size)
{
    int curr_ind = 0;
    while (payload_data[curr_ind] != ',')
    {
        curr_ind++;
    }

    char isolated_str[curr_ind + 1];
    strlcpy(isolated_str, payload_data, curr_ind);

    res_string = &payload_data[curr_ind + 2]; // We start at the next argument
    *isolated_size = curr_ind;

    // TODO: We cannot just return this because this is stack memory, find
    // another way of returning it without malloc. Probably force user to pass a
    // pointer.
    return isolated_str;
}

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
            if (strcmp(command_payload, "running_state"))
            {
                overridden_state = &running_state;
            }
            else if (strcmp(command_payload, "init_state"))
            {
                overridden_state = &init_state;
            }
            else if (strcmp(command_payload, "burn_wire_state"))
            {
                overridden_state = &burn_wire_state;
            }
            else
            {
                overridden_state = NULL;
            }

            break;
        }

        case FILE_TRANSFER:
        {
            LOG_INFO("Parsing data packet and transitioning to file transfer "
                     "state...");
            FILE_TRANSFER_DATA ft_prelim_data;
            int str_len_buf;

            // Get file name
            char file_name[PACKET_DATA_SIZE];
            strlcpy(file_name,
                    isolate_argument(command_payload, command_payload,
                                     &str_len_buf),
                    str_len_buf);
            ft_prelim_data.file_name = file_name;

            // Get chunk size
            char *str_chunk_size = isolate_argument(
                command_payload, command_payload, &str_len_buf);
            ft_prelim_data.chunk_size = atoi(str_chunk_size);

            // Get total file size
            char *str_total_file_size = isolate_argument(
                command_payload, command_payload, &str_len_buf);
            ft_prelim_data.total_file_size = atoi(str_total_file_size);

            // Get forced index
            char *str_forced_index = isolate_argument(
                command_payload, command_payload, &str_len_buf);
            ft_prelim_data.total_file_size = atoi(str_forced_index);

            LOG_INFO("Adding initial command packet to queue...");
            queue_try_add(&slate->ft_header_data, &ft_prelim_data);

            LOG_INFO("Transitioning to file_transfer state...");
            overridden_state = &file_transfer_state;
        }

        default:
            LOG_ERROR("Unknown command ID: %i", command_id);
            break;
    }
}
