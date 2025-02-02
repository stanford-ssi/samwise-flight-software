/**
 * @author  Sasha Luchyn + Michael Dalva
 * @date    2024-09-11
 *
 * Task to parse incoming bitstream and allocate data for according tasks
 */

/* How to add new command (using an example of some task1")
1. add a mnemonic code for the command 
eg: #define COMMAND1_ID 1

2. create or choose an existing data structure for this command 
eg: struct TASK1_DATA_STRUCT_FORMAT

3. create a queue in the slate.h file
eg: queue_t task1_data;

4. create a struct for a new command
eg: struct TASK1_DATA_STRUCT_FORMAT current_data_holder_task1;

5. initialize queue in the command_switch_task_init
eg: queue_init(&slate->task1_data, sizeof(current_data_holder_task1), TASK1_QUEUE_LENGTH);

*note the TASK1_QUEUE_LENGTH is the length of the queu, in other words
 how many calls for that command can be queued simultaniously*


6. add a case to the switch case in command_switch_dispatch
eg:
    case COMMAND1_ID:{
        struct TASK1_DATA_STRUCT_FORMAT task;
        uint16_t task_size = sizeof(task);
        parse_packets_as_command(slate, task_size, COMMAND1_ID, &slate->task1_data);                
        }break;


7. All set! (hopefully)
*/

#include "state_machine/tasks/command_switch_task.h"
#include "macros.h"
#include "pico/stdlib.h"
#include "slate.h"

const int RADIO_PACKETS_OUT_MAX_LENGTH = 64;   // max radio queue length
const int COMMAND_MNEMONIC_BYTE_SIZE = 1;         // how many bytes are used to identify the command
const int TASK1_QUEUE_LENGTH = 32;             // max queue length for task 1


#define PACKET_BYTE_LENGTH 251                       // in bytes TODO check?/ get from driver mod?
#define STOP_BYTE 255       // stop byte is added on the last transmission of the packet (everything afterwards is disregarded in that packet)
#define COMMAND1_ID 1
#define COMMAND2_ID 2

typedef struct
{
    uint8_t src;
    uint8_t dst;
    uint8_t flags;
    uint8_t seq;
    uint8_t len; // this should be the length of the packet structure being sent over
    uint8_t data[252];
} packet_t;

/*
 * The following structs include ALL of the data that should be stored in the non-header payload data.
 * FOR EXAMPLE: When receiving a packet, there should only be PACKET_BYTE_LENGTH that are saved into the radio queue that WE have to deal with.
 * 
 * 
 * THE FIRST COMMAND_MNEMONIC_BYTE_SIZE worth of bytes should be allocated only to the command id. so basically there are only PACKET_BYTE_LENGTH - COMMAND_MNEMONIC_BYTE_SIZE packets with struct content in them.
 */
struct TASK1_DATA_STRUCT_FORMAT
{
    int data_int_1;
    uint8_t data_byteArr_1[300];
};

struct TASK2_DATA_STRUCT_FORMAT
{
    bool yes_no;
    uint16_t number;
};


// in the end, we should replace these with just the size of the structs for each command,
// that way we don't need to create random structs just to get the size.
struct TASK1_DATA_STRUCT_FORMAT current_data_holder_task1;
struct TASK2_DATA_STRUCT_FORMAT current_data_holder_task2;


/// @brief Initialize the command switch task
/// @param slate Address of the slate
void command_switch_task_init(slate_t *slate)
{
    // initialize queue for radio input data,TODO: assumed to be initialized in the radio module and comment out when merging
    queue_init(&slate->rx_queue, PACKET_BYTE_LENGTH * sizeof(uint8_t), RADIO_PACKETS_OUT_MAX_LENGTH);

    // Initialize queues for storing processed commands
    queue_init(&slate->task1_data, sizeof(current_data_holder_task1), TASK1_QUEUE_LENGTH);
    queue_init(&slate->task2_data, sizeof(current_data_holder_task2), TASK1_QUEUE_LENGTH);

    slate->num_uploaded_bytes = 0; // Number of bytes currently uploaded to buffer
    slate->packet_buffer_index = 0; // Index of the position in payload buffer
    slate->uploading_command_id = 0; // number of command that is currently being uploaded (0 if nothing uploading)
}


/// @brief This function attempts to read the slate's incoming packet's queue in order to extract exactly 1 command. 
/// If it has not received all of the packets needed for that command, it will return false, but save its progress.
/// @param slate - The address of the slate
/// @param max_size_of_struct The size of the struct that you want to extract from the radio queue.
/// @param command_id The command ID that you found in the buffer.
bool place_packets_into_struct_buffer(slate_t *slate, uint16_t bytes_per_command, uint8_t command_id){

    // Dequeue packets into this packet buffer
    uint8_t packet_buffer[PACKET_BYTE_LENGTH]; 

    // Dequeue one packet from the queue
    bool successful_peek = queue_try_peek(&slate->rx_queue, packet_buffer); 

    // If a packet was successfully dequeued...
    if(successful_peek){
        
        // While the command upload is incomplete
        while(slate->num_uploaded_bytes < bytes_per_command){

            // Estimate the final index of the command (if there were no packet limitations)
            uint16_t estimated_end_byte_index = slate->packet_buffer_index + (bytes_per_command - slate->num_uploaded_bytes);

            // If that estimated index is too large for the current packet...
            if(estimated_end_byte_index >= PACKET_BYTE_LENGTH){
                
                // Addresses for READ -> WRITE from packet to struct
                uint8_t* packet_buffer_read = packet_buffer + slate->packet_buffer_index;
                uint8_t* struct_buffer_write = slate->struct_buffer + slate->num_uploaded_bytes;
                uint16_t length = PACKET_BYTE_LENGTH - slate->packet_buffer_index;

                // READ from packet_buffer, WRITE into the struct_buffer
                memcpy(struct_buffer_write, packet_buffer_read, length);

                // Remove the packet because we uploaded all its data.
                queue_try_remove(&slate->rx_queue, packet_buffer);

                // Update how many bytes have been uploaded
                slate->num_uploaded_bytes += length;

                // Reset the index in the buffer
                slate->packet_buffer_index = 0;

                // Attempt to retrieve the next packet in the queue...
                // ... then we can continue the uploading...
                if(!queue_try_peek(&slate->rx_queue, packet_buffer)){ // If dequeuing fails, just save where you left off.
                    slate->uploading_command_id = command_id;
                    return false;
                }
            }
            // Otherwise, the end of the command is in this packet
            else{
                // Addresses for READ -> WRITE from packet to struct
                uint8_t* packet_buffer_read = slate->struct_buffer + slate->num_uploaded_bytes;
                uint8_t* struct_buffer_write = packet_buffer + slate->packet_buffer_index;
                uint16_t length = bytes_per_command - slate->num_uploaded_bytes;

                // READ from packet_buffer, WRITE into the struct_buffer
                memcpy(struct_buffer_write, packet_buffer_read, length);
                
                // At this point the struct buffer is complete

                // Update the number of uploaded bytes
                slate->num_uploaded_bytes += length;

                // Update packet buffer index to remember where we left off
                // packet buffer index should be LESS THAN OR EQUAL TO PACKET_BYTE_SIZE at this point
                slate->packet_buffer_index += length;
            }
        }
        
        // At this point, either:
        //      1) The entire command has been received and placed into the struct buffer
        //      2) Only part of the command has been received, and we are waiting for the rest

        // If the command has been FULLY received...
        if(slate->num_uploaded_bytes == bytes_per_command){

            // If the last byte was at the end of the packet_buffer, or if the packet_buffer is reading a stop byte...
            if(slate->packet_buffer_index >= PACKET_BYTE_LENGTH || packet_buffer[slate->packet_buffer_index] == STOP_BYTE){
                
                // Discard the current packet
                queue_try_remove(&slate->rx_queue, packet_buffer);  // remove packet as the stop byte indicate that it was the last command in the packet

                // Reset the packet buffer index
                slate->packet_buffer_index = 0;
            }

            // Successfully retrieved a full command
            return true;
        }
        else{
            // Only received part of a commmand.
            return false;
        }
    }
}

/// @brief Parses the current packets as the given command and attempts to save it into the queues..
/// @param slate 
/// @param datastructure_size The size of the expected datastructure in bytes (use "sizeof(datastructure_for_command1)")
/// @param command_id Command id (Command mnenmonic)
/// @param write_address Pointer to an empty datastructure for your command
/// @param queue_pointer Pointer to the queue on the slate where to add this command to (&slat)
/// @return returns true if successfully loaded command into the appropriate task queue
bool parse_packets_as_command(slate_t* slate, queue_t* queue_pointer){
    // get whatever is the most recent packet in the slate receive queue
    packet_t packet;
    queue_try_remove(&slate->rx_queue, &packet);
    sleep_ms(500);
    LOG_INFO("was able to receive a packet out of the rx queue");

    queue_try_add(&queue_pointer, packet.data + 1);
    sleep_ms(500);
    LOG_INFO("added the packet to the proper task queue");
}

/// @brief 
/// @param slate 
void command_switch_dispatch(slate_t *slate)
{
    int number_of_commands_to_process = 1;
    for(int i = 0; i < number_of_commands_to_process; i++){
        
        // This packet will store what is currently up next in the radio receive queue
        packet_t packet;

        
        
        // Peek at the upcoming item in the radio receive queue
        bool successful_peek = queue_try_remove(&slate->rx_queue, &packet);
        
        if(successful_peek){

            for(int i = 0; i < packet.len; i++){
                LOG_INFO("packet data: %i, has value: %i", i, packet.data[i]);
            }

            /**
             * Update the command ID depending on if it was previously uploading.
             * If previously not loading (command id == 0) then reset the task byte size.
             */
            uint8_t command_id = slate->uploading_command_id;

            sleep_ms(500); 
            command_id = packet.data[0];

            sleep_ms(500);
            LOG_INFO("the command id is: %i", command_id);
            
            /**
             * Pass specific structs and taks queues appropriate for each command
             */
            switch(command_id){

                // COMMAND_ID NEED TO START FROM 1 BECAUSE 0 IS BEING USED AS THE "NOT UPLOADING" INDEX
                case COMMAND1_ID:{
                    struct TASK1_DATA_STRUCT_FORMAT task;
                    uint16_t task_size = sizeof(task);
                    parse_packets_as_command(slate, &slate->task1_data);
                        
                }break;
                case COMMAND2_ID:{
                    struct TASK2_DATA_STRUCT_FORMAT task;
                    memcpy(&task, packet.data + 1, sizeof(task));
                    sleep_ms(500);
                    LOG_INFO("copied the queue data into a structure");
                    queue_try_add(&slate->task2_data, &task);

                    LOG_INFO("struct: %i, %i", task.number, task.yes_no);
                } break;
                default:
                break;
            }
        }
    }
}

sched_task_t command_switch_task = {.name = "command_switch",
                                    .dispatch_period_ms = 100,
                                    .task_init = &command_switch_task_init,
                                    .task_dispatch = &command_switch_dispatch,
                                    /* Set to an actual value on init */
                                    .next_dispatch = 0};
