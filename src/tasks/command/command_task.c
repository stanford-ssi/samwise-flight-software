/**
 * @author  Sasha Luchyn + Michael Dalva
 * @date    2024-09-11
 *
 * Task to parse incoming bitstream and allocate data for according tasks
 */

/* How to add new command (using an example of some task1")
1. add a mnemonic code for the command
eg: #define COMMAND1_ID 1

2. create or choose an existing data structure for this command. Define it in
the packet.h file. eg: struct TASK1_DATA_STRUCT_FORMAT

3. create a queue in the slate.h file
eg: queue_t task1_data;

4. create a struct for a new command
eg: struct TASK1_DATA_STRUCT_FORMAT current_data_holder_task1;

5. initialize queue in the command_switch_task_init
eg: queue_init(&slate->task1_data, sizeof(current_data_holder_task1),
TASK1_QUEUE_LENGTH);

*note the TASK1_QUEUE_LENGTH is the length of the queu, in other words
 how many calls for that command can be queued simultaniously*


6. add a case to the switch case in command_switch_dispatch
eg:
    case COMMAND1_ID:{
        struct TASK1_DATA_STRUCT_FORMAT task;
        uint16_t task_size = sizeof(task);
        parse_packets_as_command(slate, task_size, COMMAND1_ID,
&slate->task1_data); }break;


7. All set! (hopefully)
*/

#include "command_task.h"
#include "macros.h"
#include "packet.h"
#include "pico/stdlib.h"
#include "slate.h"

const int RADIO_PACKETS_OUT_MAX_LENGTH = 64; // max radio queue length
const int COMMAND_MNEMONIC_BYTE_SIZE =
    1; // how many bytes are used to identify the command
const int TASK1_QUEUE_LENGTH = 32; // max queue length for task 1

#define PACKET_BYTE_LENGTH 251 // in bytes TODO check?/ get from driver mod?
#define STOP_BYTE                                                              \
    255 // stop byte is added on the last transmission of the packet (everything
        // afterwards is disregarded in that packet)
#define COMMAND1_ID 1
#define COMMAND2_ID 2

// in the end, we should replace these with just the size of the structs for
// each command, that way we don't need to create random structs just to get the
// size.
TASK1_DATA current_data_holder_task1;
TASK2_DATA current_data_holder_task2;

/// @brief Initialize the command switch task
/// @param slate Address of the slate
void command_task_init(slate_t *slate)
{
    // initialize queue for radio input data,TODO: assumed to be initialized in
    // the radio module and comment out when merging
    queue_init(&slate->rx_queue, PACKET_BYTE_LENGTH * sizeof(uint8_t),
               RADIO_PACKETS_OUT_MAX_LENGTH);

    // Initialize queues for storing processed commands
    queue_init(&slate->task1_data, sizeof(current_data_holder_task1),
               TASK1_QUEUE_LENGTH);
    queue_init(&slate->task2_data, sizeof(current_data_holder_task2),
               TASK1_QUEUE_LENGTH);

    slate->num_uploaded_bytes =
        0; // Number of bytes currently uploaded to buffer
    slate->packet_buffer_index = 0;  // Index of the position in payload buffer
    slate->uploading_command_id = 0; // number of command that is currently
                                     // being uploaded (0 if nothing uploading)
}

/// @brief
/// @param slate
void command_task_dispatch(slate_t *slate)
{
    int number_of_commands_to_process = 1;
    for (int i = 0; i < number_of_commands_to_process; i++)
    {

        // This packet will store what is currently up next in the radio receive
        // queue
        packet_t packet;

        // Peek at the upcoming item in the radio receive queue
        bool successful_peek = queue_try_remove(&slate->rx_queue, &packet);

        if (successful_peek)
        {
            if (!is_packet_authenticated(&packet))
            {
                // Packet is not authenticated, drop it
                LOG_ERROR("Packet authentication failed. Dropping packet.");
                continue;
            }

            /**
             * Update the command ID depending on if it was previously
             * uploading. If previously not loading (command id == 0) then reset
             * the task byte size.
             */
            uint8_t command_id = packet.data[0];
            LOG_INFO("Command ID Recieved: %i", command_id);

            /**
             * Pass specific structs and taks queues appropriate for each
             * command
             */
            switch (command_id)
            {
                // COMMAND_ID NEED TO START FROM 1 BECAUSE 0 IS BEING USED AS
                // THE "NOT UPLOADING" INDEX
                case COMMAND1_ID:
                {
                    TASK1_DATA task;
                    memcpy(&task, packet.data + 1, sizeof(task));
                    queue_try_add(&slate->task1_data, &task);
                    LOG_INFO("struct 1: %i, %i", task);
                }
                break;
                case COMMAND2_ID:
                {
                    TASK2_DATA task;
                    memcpy(&task, packet.data + 1, sizeof(task));
                    queue_try_add(&slate->task2_data, &task);
                    LOG_INFO("struct 2: %i, %i", task.number, task.yes_no);
                }
                break;
                default:
                    break;
            }
        }
    }
}

sched_task_t command_task = {.name = "command",
                             .dispatch_period_ms = 100,
                             .task_init = &command_task_init,
                             .task_dispatch = &command_task_dispatch,
                             /* Set to an actual value on init */
                             .next_dispatch = 0};
