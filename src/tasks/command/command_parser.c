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
#include "config.h"
#include "macros.h"
#include "payload_uart.h"
#include "rfm9x.h"
#include "states.h"
#include <stdio.h>
#include <string.h>

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
        /* FTP Commands */
        case FTP_START_FILE_WRITE:
        {
            LOG_INFO("FTP_START_FILE_WRITE command received.");

            if (slate->filesys_is_writing_file)
            {
                LOG_ERROR("A file is already being written. Cannot "
                          "start a new file write. Existing fname = %s, "
                          "new fname = %u",
                          slate->filesys_buffered_fname,
                          ((unsigned char)command_payload[0] << 8) |
                              (unsigned char)command_payload[1]);
                break;
            }

            if (queue_is_full(&slate->ftp_start_file_write_data))
            {
                LOG_ERROR(
                    "VERY BAD ERROR! FTP start file write queue is "
                    "full, but filesys_is_writing_file is false. Dropping "
                    "command.");
                break;
            }

            FTP_START_FILE_WRITE_DATA command_data;

            memcpy(&command_data.fname, command_payload,
                   sizeof(FILESYS_BUFFERED_FNAME_T));

            memcpy(&command_data.file_len,
                   command_payload + sizeof(FILESYS_BUFFERED_FNAME_T),
                   sizeof(FILESYS_BUFFERED_FILE_LEN_T));

            memcpy(&command_data.file_crc,
                   command_payload + sizeof(FILESYS_BUFFERED_FNAME_T) +
                       sizeof(FILESYS_BUFFERED_FILE_LEN_T),
                   sizeof(FILESYS_BUFFERED_FILE_CRC_T));

            if (!queue_try_add(&slate->ftp_start_file_write_data,
                               &command_data))
            {
                LOG_ERROR("Failed to enqueue FTP_START_FILE_WRITE_DATA "
                          "command for fname %s",
                          command_data.fname);
            }

            break;
        }
        case FTP_WRITE_TO_FILE:
        {
            LOG_INFO("FTP_WRITE_TO_FILE command received.");
            FTP_WRITE_TO_FILE_DATA command_data;

            // This should be the size of the data payload in this packet
            // TODO: THIS IS INCORRECT! Look at
            // https://github.com/stanford-ssi/samwise-flight-software/issues/204
            command_data.data_len = packet->len - COMMAND_MNEMONIC_SIZE -
                                    sizeof(FILESYS_BUFFERED_FNAME_T) -
                                    sizeof(FTP_PACKET_SEQUENCE_T);

            if (command_data.data_len > FTP_DATA_PAYLOAD_SIZE)
            {
                LOG_ERROR("Packet data size exceeds FTP_DATA_PAYLOAD_SIZE! "
                          "Something went really wrong.");
                break;
            }

            memcpy(&command_data.fname, command_payload,
                   sizeof(FILESYS_BUFFERED_FNAME_T));

            memcpy(&command_data.packet_id,
                   command_payload + sizeof(FILESYS_BUFFERED_FNAME_T),
                   sizeof(FTP_PACKET_SEQUENCE_T));

            memcpy(
                &command_data.data,
                command_payload + sizeof(FILESYS_BUFFERED_FNAME_T) +
                    sizeof(FTP_PACKET_SEQUENCE_T),
                command_data.data_len); // Use data_len for last packet - may be
                                        // smaller than FTP_DATA_PAYLOAD_SIZE

            if (!queue_try_add(&slate->ftp_write_to_file_data, &command_data))
            {
                LOG_ERROR("Failed to enqueue FTP_WRITE_TO_FILE_DATA command "
                          "for fname %s, packet_id %u",
                          command_data.fname, command_data.packet_id);
            }

            break;
        }
        case FTP_CANCEL_FILE_WRITE:
        {
            LOG_INFO("FTP_CANCEL_FILE_WRITE command received.");

            if (!slate->filesys_is_writing_file)
            {
                LOG_ERROR("No file is currently being written. Cannot cancel "
                          "non-existent file write.");
                break;
            }

            FTP_CANCEL_FILE_WRITE_DATA command_data;

            memcpy(&command_data.fname, command_payload,
                   sizeof(FILESYS_BUFFERED_FNAME_T));

            if (command_data.fname != slate->filesys_buffered_fname)
            {
                LOG_ERROR("FTP_CANCEL_FILE_WRITE command fname %s does not "
                          "match current writing fname %s. Ignoring cancel "
                          "command.",
                          command_data.fname, slate->filesys_buffered_fname);
                break;
            }

            if (!queue_try_add(&slate->ftp_cancel_file_write_data,
                               &command_data))
            {
                LOG_ERROR("Failed to enqueue FTP_CANCEL_FILE_WRITE_DATA "
                          "command for fname %s",
                          command_data.fname);
            }

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
