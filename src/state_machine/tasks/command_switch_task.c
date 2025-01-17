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

*note the TASK1_QUEUE_LENGTH is the length of the queu, in other words
 how many calls to that function can be queued simultaniously*


6. add a case to the switch case in command_switch_dispatch
eg:
case Func1_MNEM:{
                    // Create a temporary datastructure appropriate for the task to add things into the queue.
                    struct TASK1_DATA_STRUCT_FORMAT task;
                    
                    // How many bytes are in the struct we want? 
                    uint16_t task_size = sizeof(task);
                    
                    // Reads whatever was in the package into the slate->buffer
                    enqueue_function_from_buffer(slate, task_size, Func1_MNEM, &slate->task1_data);
                        
                }break;


7. All set! (hopefully)
*/

#include "state_machine/tasks/command_switch_task.h"
#include "macros.h"
#include "pico/stdlib.h"
#include "slate.h"

const int RADIO_PACKETS_OUT_MAX_LENGTH = 64;   // max radio queue length
const int FUNC_MNEMONIC_BYTE_SIZE = 1;         // how many bytes are used to identify the function
const int TASK1_QUEUE_LENGTH = 32;             // max queue length for task 1


#define PAYLOAD_SIZE 251                       // in bytes TODO check?/ get from driver mod?
#define STOP_MNEM 255       // stop byte is added on the last transmission of the packet (everything afterwards is disregarded in that packet)
#define FUNC1_ID 1
#define FUNC2_ID 2

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


/// @brief Initialize the command switch task
/// @param slate Address of the slate
void command_switch_task_init(slate_t *slate)
{
    // Initialize whatever queue that has all of the packets
    queue_init(&slate->radio_packets_out, PAYLOAD_SIZE * sizeof(uint8_t), RADIO_PACKETS_OUT_MAX_LENGTH);

    /*
        Now, initialize queues for every function.
        Every time a command is sent to execute a certain function, add the request to the queue.
    */
    queue_init(&slate->task1_data, sizeof(current_data_holder_task1), TASK1_QUEUE_LENGTH);
    queue_init(&slate->task2_data, sizeof(current_data_holder_task2), TASK1_QUEUE_LENGTH);

    /*
        Initialize information that tracks how packets are received and uploaded
    */
    slate->current_task_byte_size = 0;
    slate->current_byte_index = 0;
    slate->uploading_function_number = 0;
}


/// @brief This function attempts to read the slate's incoming packet's queue in order to extract exactly 1 function. 
/// If it has not received all of the packets needed for that function, it will return false, but save its progress.
/// @param slate - The address of the slate
/// @param max_size_of_struct The size of the struct that you want to extract from the radio queue.
/// @param function_id The function ID that you found in the buffer.
bool read_function_into_buffer(slate_t *slate, uint16_t max_size_of_struct, uint8_t function_id){


    uint8_t payload[PAYLOAD_SIZE]; // The payload to be read

    /*
        Get a payload from the queue
    */
    bool successful_peek = queue_try_peek(&slate->radio_packets_out, payload); 


    if(successful_peek){
        
        /*
            Loop through packets to get command bytes.
            False if not enough packets.
        */
        while(slate->current_task_byte_size < max_size_of_struct){
            uint16_t estimated_end_byte_index = slate->current_byte_index + (max_size_of_struct - slate->current_task_byte_size);

            /*
                If the struct is too large for the current packet...
                    1) Save everything left in the packet
                    2) Discard the packet
                    3) move on to the next packet (false if no more packets)
            */
            if(estimated_end_byte_index >= PAYLOAD_SIZE){
                /*
                    Copy bytes until the end of the packet into a struct.
                    Remove the packet when done.
                */
                uint8_t* where_to_place_into_struct = slate->buffer + slate->current_task_byte_size;
                uint8_t* where_to_read_from_payload = payload + slate->current_byte_index;
                uint16_t length = PAYLOAD_SIZE - slate->current_byte_index;
                memcpy(where_to_place_into_struct, where_to_read_from_payload, length);
                queue_try_remove(&slate->radio_packets_out, payload);

                /**
                 * Increment byte progress size, but reset the buffer index.
                 */
                slate->current_task_byte_size += length;
                slate->current_byte_index = 0;

                /*
                    If the next packet is missing, keep track of what function we are looking for.
                    If the packet exists, save it into payload and repeat.
                */
                if(!queue_try_peek(&slate->radio_packets_out, payload)){
                    slate->uploading_function_number = function_id;
                    return false;
                }
            }
            /**
             * Otherwise, the end of the command is somewhere in this packet.
             */
            else{ 
                /**
                 * Determine where to read bytes into the struct
                 */
                uint8_t* where_to_place_into_struct = slate->buffer + slate->current_task_byte_size;
                uint8_t* where_to_read_from_payload = payload + slate->current_byte_index;
                uint16_t length = max_size_of_struct - slate->current_task_byte_size;
                memcpy(where_to_place_into_struct, where_to_read_from_payload, length);
                
                /**
                 * Update tracking variables
                 */
                slate->current_task_byte_size += length;
                slate->current_byte_index += length;
            }
        }
        
        /**
         * Finally, if the command has been fully received...
         * potentially you will have to remove it.
         */
        if(slate->current_task_byte_size == max_size_of_struct){
            if(slate->current_byte_index >= PAYLOAD_SIZE || payload[slate->current_byte_index] == STOP_MNEM){
                queue_try_remove(&slate->radio_packets_out, payload);
                slate->current_byte_index = 0;
            }
            return true;
        }
        else{
            LOG_DEBUG("function data not fully received");
            return false;
        }
    }
}

/// @brief Parses the current packets as the given function and attempts to save it into the queues..
/// @param slate 
/// @param datastructure_size The size of the expected datastructure in bytes (use "sizeof(datastructure_for_function1)")
/// @param func_id Function id (Function mnenmonic) 
/// @param write_address Pointer to an empty datastructure for your function
/// @param queue_pointer Pointer to the queue on the slate where to add this function to (&slat)
/// @return returns true if successfully loaded function into the appropriate task queue
bool parse_packets_as_command(slate_t* slate, uint16_t datastructure_size, uint8_t func_id, void* queue_pointer){
    
    // returns 0 if there was an error reading the function into the buffer
    if(!read_function_into_buffer(slate, datastructure_size, func_id)) return 0;

    if(slate->current_task_byte_size == datastructure_size){
        slate->current_task_byte_size = 0;

        // Set the currently uploading function to 0 (no functions are in the process of being uploaded)
        slate->uploading_function_number = 0;

        queue_try_add(queue_pointer, slate->buffer);
        return true;   
    }

}

/// @brief 
/// @param slate 
void command_switch_dispatch(slate_t *slate)
{
    int number_of_commands_to_process = 1;
    for(int i = 0; i < number_of_commands_to_process; i++){
        
        // This payload will store what is currently up next in the radio receive queue
        uint8_t payload[PAYLOAD_SIZE];
        
        // Peek at the upcoming item in the radio receive queue
        bool successful_peek = queue_try_peek(&slate->radio_packets_out, payload);
        
        if(successful_peek){
            /**
             * Update the function ID depending on if it was previously uploading.
             * If previously not loading (function id == 0) then reset the task byte size.
             */
            uint8_t function_id = slate->uploading_function_number;
            if(slate->uploading_function_number == 0){
                function_id = payload[slate->current_byte_index];
                slate->current_byte_index += FUNC_MNEMONIC_BYTE_SIZE;
                slate->current_task_byte_size = 0;
            }
            
            /**
             * Perform slightly different operations for each function
             */
            switch(function_id){

                // THESE NEED TO START FROM 1 BECAUSE 0 IS BEING USED AS THE "NOT UPLOADING" INDEX
                case FUNC1_ID:{
                    struct TASK1_DATA_STRUCT_FORMAT task;
                    uint16_t task_size = sizeof(task);
                    parse_packets_as_command(slate, task_size, FUNC1_ID, &slate->task1_data);
                        
                }break;
                case FUNC2_ID:{
                    struct TASK2_DATA_STRUCT_FORMAT task;
                    uint16_t task_size = sizeof(task);
                    parse_packets_as_command(slate, task_size, FUNC2_ID, &slate->task2_data);
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
