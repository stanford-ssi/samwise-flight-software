/**
 * @author  Sasha Luchyn + Michael Dalva
 * @date    2024-09-11
 *
 * Task to parse incoming bitstream and allocate data for according tasks
 */

/* How to add new function (using an example of some task1")
1. add a mnemonic code for the function 
eg: #define Func1_MNEM 1

2. create or choose an existing data structure for this function 
eg: struct TASK1_DATA_STRUCT_FORMAT

3. create a queue in the slate.h file
eg: queue_t task1_data;

4. create a struct for a new function
eg: struct TASK1_DATA_STRUCT_FORMAT current_data_holder_task1;

5. initialize queue in the command_switch_task_init
eg: queue_init(&slate->task1_data, sizeof(current_data_holder_task1), TASK1_QUEUE_LENGTH);

6. add a case to the switch case in command_switch_dispatch
eg:
case Func1_MNEM:{

                        // Create a temporary task1 to add things into the queue.
                        struct TASK1_DATA_STRUCT_FORMAT task1;
                        
                        // How many bytes are in the struct we want? 
                        uint16_t max_size_task1 = sizeof(task1);
                        
                        // Reads whatever was in the package into the slate->buffer
                        read_function_into_memory(slate, max_size_task1, Func1_MNEM);

                        if(slate->current_task_byte_size == max_size_task1){
                            slate->current_task_byte_size = 0;
                            // memcopy the things in the buffer into a struct
                            memcpy(&task1, slate->buffer, max_size_task1);

                            // Set the currently uploading function to 0 (no functions are in the process of being uploaded)
                            slate->uploading_function_number = 0;

                            // AT THIS POINT, AN ENTIRE STRUCT HAS BEEN READ, SO WE WILL ADD IT HERE !
                            queue_try_add(&slate->task1_data, &task1);
                        }
                        
                }break;


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
    uint8_t data_byteArr_1[300];
};

struct TASK2_DATA_STRUCT_FORMAT
{
    bool yes_no;
    uint16_t number;
};




// in the end, we should replace these with just the size of the structs for each function,
// that way we don't need to create random structs just to get the size.
struct TASK1_DATA_STRUCT_FORMAT current_data_holder_task1;
struct TASK2_DATA_STRUCT_FORMAT current_data_holder_task2;

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
    queue_init(&slate->task2_data, sizeof(current_data_holder_task2), TASK1_QUEUE_LENGTH);

    const uint8_t MAX = 10;

    slate->buffer[MAX];

    slate->current_task_byte_size = 0;
    slate->current_byte_index = 0;
    slate->uploading_function_number = 0;
}


/// @brief This function attempts to read the slate's incoming packet's queue in order to extract exactly 1 function. 
/// If it has not received all of the packets, it will return false, but save its progress.
/// @param slate - The address of the slate
/// @param max_size_of_struct The size of the struct that you want to extract from the radio queue.
/// @param function_id The function ID that you found in the buffer.
bool read_function_into_buffer(slate_t *slate, uint16_t max_size_of_struct, uint8_t function_id){

    // Create the payload that is about to be read
    uint8_t payload[PAYLOAD_SIZE];

    // Peek at the upcoming item in the radio receive queue
    bool successful_peek = queue_try_peek(&slate->radio_packets_out, payload);

    if(successful_peek){

        // Get the function id from the first byte that we are reading from.
        //uint8_t function_id = payload[slate->current_byte_index];
        
        // While struct only partially filled
        while(slate->current_task_byte_size < max_size_of_struct){

            // Check how far the struct WOULD go in bytes
            uint16_t estimated_end_byte_index = slate->current_byte_index + (max_size_of_struct - slate->current_task_byte_size);


            // If the struct would end AFTER the payload size 
            if(estimated_end_byte_index >= PAYLOAD_SIZE){
                
                // Calculate where to read from the packet to place into the struct
                uint8_t* where_to_place_into_struct = slate->buffer + slate->current_task_byte_size;
                uint8_t* where_to_read_from_payload = payload + slate->current_byte_index;
                uint16_t length = PAYLOAD_SIZE - slate->current_byte_index;


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
                    LOG_INFO("discarding packet, reading next");

                    // Make it the new "payload"
                    memcpy(payload, next_payload, PAYLOAD_SIZE * sizeof(uint8_t));

                    // reset the slate->current_byte_index
                    slate->current_byte_index = 0;
                }
                else{
                    LOG_INFO("discarding packet, still waiting for next packet");
                    // If there is no next payload, it probably hasn't come yet, so just break
                    // The next for loops will just not do anything cus it won't be able to peek new data.
                    slate->current_byte_index = 0;

                    // Set the function uploading number to the current function that is being transmitted
                    slate->uploading_function_number = function_id;
                    return false;
                }
            }

            else{ // If the function is fully contained in the packet, then do the following:
                // COPY ALL OF THE PARTIAL DATA FROM THE PAYLOAD INTO THE STRUCT
                uint8_t* where_to_place_into_struct = slate->buffer + slate->current_task_byte_size;
                uint8_t* where_to_read_from_payload = payload + slate->current_byte_index;
                uint16_t length = max_size_of_struct - slate->current_task_byte_size;

                LOG_INFO("max size of struct: %i, current task byte size: %i", max_size_of_struct, slate->current_task_byte_size);
                // Copy partial data into the struct
                memcpy(where_to_place_into_struct, where_to_read_from_payload, length);

                for(int i =0; i < length; i++){
                    LOG_INFO("at index %i, the byte is: %i", i, where_to_read_from_payload[i]);
                }
                
                // AT THIS POINT, THE STRUCT SHOULD BE COMPLETED !!! RAHHH !!!
                
                // place the struct into the queue

                // Update the size of task1_current_byte_size
                slate->current_task_byte_size += length;

                // Update the current byte index so that it is now pointing at where the next function should be.
                slate->current_byte_index += length;
            }
        }

        // If the full package was retrieved
        if(slate->current_task_byte_size == max_size_of_struct){
            
            LOG_INFO("we received a full command, and we are now left at index: %i", slate->current_byte_index);
            // if the full package was retrieved, and then next thing is a stop, then stop
            if(slate->current_byte_index >= PAYLOAD_SIZE || payload[slate->current_byte_index] == STOP_MNEM){
                LOG_INFO("discarding packet, stop byte.");
                // Delete the packet
                queue_try_remove(&slate->radio_packets_out, payload);

                // Reset it so the last place on the packet is 0.
                slate->current_byte_index = 0;
            }
            else{
                LOG_INFO("not discarding the packet cus there might be something else, leaving off at: %i", slate->current_byte_index);
            }

            return true;
        }
        else{
            LOG_INFO("command not fully received");
            return false;
        }
    }
}

/// @brief Reads bytes from a buffer into a datastructure, which is then placed into its task queue.
/// @param slate 
/// @param datastructure_size The size of the expected datastructure in bytes (use "sizeof(datastructure_for_function1)")
/// @param func_id Function id (Function mnenmonic) 
/// @param write_address Pointer to an empty datastructure for your function
/// @param queue_pointer Pointer to the queue on the slate where to add this function to (&slat)
/// @return returns true if successfully loaded function into the appropriate task queue
bool enqueue_function_from_buffer(slate_t* slate, uint16_t datastructure_size, uint8_t func_id, void* queue_pointer){
    
    // returns 0 if there was an error reading the function into the buffer
    if(!read_function_into_buffer(slate, datastructure_size, func_id)) return 0;

    if(slate->current_task_byte_size == datastructure_size){
        slate->current_task_byte_size = 0;

        // Set the currently uploading function to 0 (no functions are in the process of being uploaded)
        slate->uploading_function_number = 0;

        queue_try_add(queue_pointer, slate->buffer);
    }

    return true;   
}


// This should read "number of commands" from the queue, -1 for all.
//
// First it will peek for whatever packet is incoming. If it realizes that it has an entire function that can be taken, it will simply take it.
// If it has only a function, it will try to save the fact that it's looking for the second part of the function.
// If it has the second part of a function, it will try to complete whatever was the previous function.
void command_switch_dispatch(slate_t *slate)
{
    int number_of_commands_to_process = 1;
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

            LOG_INFO("{radio} this is what function was received: %i", function_id);

            switch(function_id){

                // THESE NEED TO START FROM 1 BECAUSE 0 IS BEING USED AS THE "NOT UPLOADING" INDEX
                case Func1_MNEM:{
                    // Create a temporary task1 to add things into the queue.
                    struct TASK1_DATA_STRUCT_FORMAT task;
                    
                    // How many bytes are in the struct we want? 
                    uint16_t task_size = sizeof(task);
                    
                    // Reads whatever was in the package into the slate->buffer
                    enqueue_function_from_buffer(slate, task_size, Func1_MNEM, &slate->task1_data);
                        
                }break;
                case 2:{
                    // Create a temporary task1 to add things into the queue.
                    struct TASK2_DATA_STRUCT_FORMAT task;
                    
                    // How many bytes are in the struct we want? 
                    uint16_t task_size = sizeof(task);

                    LOG_INFO("task size is %i", task_size);
                    
                    // Reads whatever was in the package into the slate->buffer
                    enqueue_function_from_buffer(slate, task_size, 2, &slate->task2_data);
                } break;
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
