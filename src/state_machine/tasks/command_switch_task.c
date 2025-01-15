/**
 * @author  Sasha Luchyn
 * @date    2024-09-11
 *
 * Task to parse incoming bitstream and allocate data for according tasks
 * This is a very rough sketch of how it should work, nothing works
 */

/* How to add new function (using an example of some task1")
1. add a mnemonic code for the function 
eg: #define Func1_MNEM 1

2. create or choose an existing data structure for this function 
eg: struct TASK1_DATA_STRUCT_FORMAT

3. create a queue in the slate.h file
eg: queue_t task1_data;

4. create a struct for a new function
eg: struct TASK1_DATA_STRUCT_FORMAT T1DS;

5. initialize queue in the command_switch_task_init
eg: queue_init(&slate->task1_data, sizeof(T1DS), TASK1_QUEUE_LENGTH);

6. add a case to the switch case in command_switch_dispatch
eg (old): 
case Func1_MNEM: 
                    // copy to struct for the first function sizeof(struct) bytes
                    // from payload array starting at payload_head + 1
                    memcpy(&T1DS, &payload + payload_head + FUNC_MNEMONIC_BYTE_SIZE, sizeof(T1DS));
                    queue_try_add(&slate->task1_data, &T1DS);       // adding the task data to the appropriate queue
                    payload_head += FUNC_MNEMONIC_BYTE_SIZE;        // move for func mnem 
                    payload_head += sizeof(T1DS);                   // move for data size 
                    break;

eg (new, with wrapper): 
case Func1_MNEM: // TODO test this shit,
                                 // TODO evaluate the need of supporting data of len more than
                                 // the space left in the packet

                    struct TASK1_DATA_STRUCT_FORMAT* T1DS_count = &T1DS;
                    // support for data longer than the space left in a packet (potentially useless?)
                    while (payload_head + FUNC_MNEMONIC_BYTE_SIZE + sizeof(T1DS) >= PAYLOAD_SIZE ){
                        // copy max length, modify payload head and try to dequeue a new payload
                        memcpy(&T1DS_count, &payload + payload_head + FUNC_MNEMONIC_BYTE_SIZE,
                                     PAYLOAD_SIZE - payload_head - FUNC_MNEMONIC_BYTE_SIZE); // copy the rest of the packet
                        T1DS_count += PAYLOAD_SIZE - payload_head - FUNC_MNEMONIC_BYTE_SIZE;    // move pointer of the DS
                        queue_try_remove(&slate->radio_packets_out, &payload);   // get next packet
                        payload_head = 0;                                       // start next packet from start
                    }

                    // copy to struct for the first function sizeof(struct) bytes
                    // from payload array starting at payload_head + 1
                    memcpy(&T1DS_count, &payload + payload_head + FUNC_MNEMONIC_BYTE_SIZE, sizeof(T1DS) % PAYLOAD_SIZE);
                    queue_try_add(&slate->task1_data, &T1DS);       // adding the task data to the appropriate queue
                    payload_head += FUNC_MNEMONIC_BYTE_SIZE;        // move for func mnem 
                    payload_head += sizeof(T1DS) % PAYLOAD_SIZE;                   // move for data size 
                    break;

7. All set! (hopefully)

*/

// Function Mnemonics FuncName assignedNumber
// assignedNumber < (2^(FUNC_MNEMONIC_BYTE_SIZE)*8) -1
#define STOP_MNEM 255       // stop byte is added on the last transmission of the packet
#define Func1_MNEM 1
//.....
//#define Func254_MNEM 254


#include "state_machine/tasks/command_switch_task.h"
#include "macros.h"
#include "pico/stdlib.h"
#include "slate.h"

#define PAYLOAD_SIZE 251                     // in bytes TODO check?/ get from driver mod?
const int RADIO_PACKETS_OUT_MAX_LENGTH = 32; // max queue length
const int FUNC_MNEMONIC_BYTE_SIZE = 1;
const int TASK1_QUEUE_LENGTH = 32; // max queue length for task 1



/*
 * The following structs include ALL of the data that should be stored in the non-header payload data.
 * FOR EXAMPLE: When receiving a packet, there should only be PAYLOAD_SIZE that are saved into the radio queue that WE have to deal with.
 * 
 * 
 * THE FIRST FUNC_MNEMONIC_BYTE_SIZE worth of bytes should be allocated only to the function id. so basically there are only PAYLOAD_SIZE - FUNC_MNEMONIC_BYTE_SIZE packets with struct content in them.
 */
struct TASK1_DATA_STRUCT_FORMAT
{
    int data_int_1;
    uint8_t data_byteArr_1[16];
};

struct TASK2_DATA_STRUCT_FORMAT
{
    int data_int_1;
    uint8_t data_byteArr_1[16];
};




// in the end, we should replace these with just the size of the structs for each function,
// that way we don't need to create random structs just to get the size.
struct TASK1_DATA_STRUCT_FORMAT current_data_holder_task1;
struct TASK1_DATA_STRUCT_FORMAT current_data_holder_task2;

uint8_t task1_current_byte_size = 0;
uint8_t task2_current_byte_size = 0;

uint8_t maximum_struct_allocation = 0;
// What is the struct

void command_switch_task_init(slate_t *slate)
{
    // initialize queue for radio input data (TODO: potentially move to radio
    // stuff)
    queue_init(&slate->radio_packets_out, PAYLOAD_SIZE * sizeof(uint8_t),
               RADIO_PACKETS_OUT_MAX_LENGTH);

    // initialize queue for task 1
    queue_init(&slate->task1_data, sizeof(current_data_holder_task1), TASK1_QUEUE_LENGTH);

    const uint8_t MAX = 10;

    slate->buffer[MAX];

    slate->current_task_byte_size = 0;
    slate->current_byte_index = 0;
    slate->uploading_function_number = 0;
}


void read_function_into_memory(slate_t *slate, uint8_t max_size_of_struct){

    LOG_INFO("reading the function into the buffer");
    LOG_INFO("byte index before: %i", slate->current_byte_index);

    // Create the payload that is about to be read
    uint8_t payload[PAYLOAD_SIZE];

    // Peek at the upcoming item in the radio receive queue
    bool successful_peek = queue_try_peek(&slate->radio_packets_out, payload);

    if(successful_peek){

        // Get the function id from the first byte that we are reading from.
        //uint8_t function_id = payload[slate->current_byte_index];
        
        // While struct only partially filled
        while(slate->current_task_byte_size < max_size_of_struct){
            LOG_INFO("Current byte index: %i", slate->current_byte_index);

            // Check how far the struct WOULD go in bytes
            uint8_t estimated_end_byte_index = slate->current_byte_index + (max_size_of_struct - slate->current_task_byte_size);
            
            // If the struct would end AFTER the payload size 
            if(estimated_end_byte_index >= PAYLOAD_SIZE){
                LOG_INFO("Function data is incomplete, checking for rest of command");
                
                // Calculate where to read from the packet to place into the struct
                uint8_t* where_to_place_into_struct = slate->buffer + slate->current_task_byte_size;
                uint8_t* where_to_read_from_payload = payload + slate->current_byte_index;
                uint8_t length = PAYLOAD_SIZE - slate->current_byte_index;


                // Copy partial data into the struct
                memcpy(where_to_place_into_struct, where_to_read_from_payload, length);
                
                // Increase task1_current_byte_size 
                slate->current_task_byte_size += length;

                // Dequeue the packet because we have exhausted it.
                queue_try_remove(&slate->radio_packets_out, payload);
                
                // Check if there is a next payload
                uint8_t next_payload[PAYLOAD_SIZE];
                bool successful_next_peek = queue_try_peek(&slate->radio_packets_out, next_payload);
                
                // If there is a next payload
                if(successful_next_peek){
                    LOG_INFO("Reading into the next packet successfully");

                    // Make it the new "payload"
                    memcpy(payload, next_payload, PAYLOAD_SIZE * sizeof(uint8_t));

                    // reset the slate->current_byte_index
                    slate->current_byte_index = 0;
                }
                else{
                    LOG_INFO("Rest of command not found, will try again later");
                    // If there is no next payload, it probably hasn't come yet, so just break
                    // The next for loops will just not do anything cus it won't be able to peek new data.
                    slate->current_byte_index = 0;
                    break;
                }
            }

            else{ // If the function is fully contained in the packet, then do the following:
                LOG_INFO("Function data found in completion");

                sleep_ms(1000);

                LOG_INFO("how big the current_task_byte_size is: %i", slate->current_task_byte_size);

                sleep_ms(100);

                // COPY ALL OF THE PARTIAL DATA FROM THE PAYLOAD INTO THE STRUCT
                uint8_t* where_to_place_into_struct = slate->buffer + slate->current_task_byte_size;
                sleep_ms(100);
                uint8_t* where_to_read_from_payload = payload + slate->current_byte_index;
                sleep_ms(100);
                uint8_t length = max_size_of_struct - slate->current_task_byte_size;
                sleep_ms(100);

                LOG_INFO("Reading from where in the payload: %i", slate->current_byte_index);
                LOG_INFO("how long to read for: %i", length);
                
                LOG_INFO("How far we should be into the current task bytes: %i", slate->current_task_byte_size);

                sleep_ms(500);

                // Copy partial data into the struct
                memcpy(where_to_place_into_struct, where_to_read_from_payload, length);

                for(int i =0; i < 20; i++){
                    LOG_INFO("at index: %i, the byte is: %i", i, slate->buffer[i]);
                }
                
                // AT THIS POINT, THE STRUCT SHOULD BE COMPLETED !!! RAHHH !!!
                
                // place the struct into the queue

                // Update the size of task1_current_byte_size
                slate->current_task_byte_size += length;
                LOG_INFO("Length of data saved: %i", slate->current_task_byte_size);

                // Update the current byte index so that it is now pointing at where the next function should be.
                slate->current_byte_index += length;
            }
        }

        // If the full package was retrieved
        if(slate->current_task_byte_size == max_size_of_struct){
            LOG_INFO("data full received");

            // if the full package was retrieved, and then next thing is a stop, then stop
            if(slate->current_byte_index >= PAYLOAD_SIZE || payload[slate->current_byte_index] == STOP_MNEM ){
                LOG_INFO("deleting package because it either stopped or is fully read");
                // Delete the packet
                queue_try_remove(&slate->radio_packets_out, payload);

                // Reset it so the last place on the packet is 0.
                slate->current_byte_index = 0;
            }
        }
    }
}


// This should read "number of commands" from the queue, -1 for all.
//
// First it will peek for whatever packet is incoming. If it realizes that it has an entire function that can be taken, it will simply take it.
// If it has only a function, it will try to save the fact that it's looking for the second part of the function.
// If it has the second part of a function, it will try to complete whatever was the previous function.
void command_switch_dispatch(slate_t *slate)
{
    int number_of_commands_to_process = 1;
    LOG_INFO("Beginning command dispatch");
    for(int i = 0; i < number_of_commands_to_process; i++){
        
        // This payload will store what is currently up next in the radio receive queue
        uint8_t payload[PAYLOAD_SIZE];
        
        // Peek at the upcoming item in the radio receive queue
        bool successful_peek = queue_try_peek(&slate->radio_packets_out, payload);
        
        if(successful_peek){
            
            // Set the function id to whatever was uploading before...
            uint8_t function_id = slate->uploading_function_number;

            // ...unless there was nothing uploading.
            if(slate->uploading_function_number == 0){
                function_id = payload[slate->current_byte_index];
                slate->current_byte_index += FUNC_MNEMONIC_BYTE_SIZE;
                slate->current_task_byte_size = 0;
            }

            LOG_INFO("Received a packet indicating function %i", function_id);

            switch(function_id){

                // THESE NEED TO START FROM 1 BECAUSE 0 IS BEING USED AS THE "NOT UPLOADING" INDEX
                case Func1_MNEM:
                    // Create a temporary task1 to add things into the queue.
                    struct TASK1_DATA_STRUCT_FORMAT task1;

                    // How many bytes are in the struct we want? 
                    uint8_t max_size_task1 = sizeof(task1);
                    
                    // Reads whatever was in the package into the slate->buffer
                    read_function_into_memory(slate, max_size_task1);
                    
                    LOG_INFO("current task byte task: %i", slate->current_task_byte_size);
                    LOG_INFO("max size of task 1: %i", max_size_task1);

                    if(slate->current_task_byte_size == max_size_task1){
                        slate->current_task_byte_size = 0;
                        // memcopy the things in the buffer into a struct
                        memcpy(&task1, slate->buffer, max_size_task1);
                        for(int i =0; i < 20; i++){
                            LOG_INFO("copying data structure.. at index: %i, the byte is: %i", i, slate->buffer[i]);
                        }

                        // AT THIS POINT, AN ENTIRE STRUCT HAS BEEN READ, SO WE WILL ADD IT HERE !
                        queue_try_add(&slate->task1_data, &task1);
                    }
                    break;
                //case Func2_MNEM:
                //    break;
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
