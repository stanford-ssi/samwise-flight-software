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
}

const uint8_t number_of_commands = 1;

//void *ptr;

const uint8_t MAX = 10;

uint8_t buffer[MAX];

uint8_t current_task_byte_size = 0;
uint8_t current_byte_index = 0;
uint8_t last_place_on_packet = 0;

void read_function_into_memory(slate_t *slate, uint8_t max_size_of_struct){

    // If no bytes have been read yet
    if(current_task_byte_size == 0){
        // allocate some new space for the struct to be saved into
        //ptr = malloc(sizeof(current_data_holder_task1));
    }
    
    // Tell the machine where to start reading bytes
    current_byte_index = last_place_on_packet;

    // Create the payload that is about to be read
    uint8_t payload[PAYLOAD_SIZE];

     // Peek at the upcoming item in the radio receive queue
    bool successful_peek = queue_try_peek(&slate->radio_packets_out, &payload);

    if(successful_peek){

        // Get the function id from the first byte that we are reading from.
        uint8_t function_id = payload[current_byte_index];

        // Increment the byte index forward by the function mnemonic size
        // This means you can start reading data from the next byte
        current_byte_index += FUNC_MNEMONIC_BYTE_SIZE;
        
        // While struct only partially filled
        while(current_task_byte_size < max_size_of_struct){

            // Check how far the struct WOULD go in bytes
            uint8_t estimated_end_byte_index = current_byte_index + (max_size_of_struct - current_task_byte_size);
            
            // If the struct would end AFTER the payload size 
            if(estimated_end_byte_index >= PAYLOAD_SIZE){
                
                // Calculate where to read from the packet to place into the struct
                uint8_t* where_to_place_into_struct = &buffer + current_task_byte_size;
                uint8_t* where_to_read_from_payload = &payload + current_byte_index;
                uint8_t length = PAYLOAD_SIZE - current_byte_index;


                // Copy partial data into the struct
                memcpy(where_to_place_into_struct, where_to_read_from_payload, length);
                
                // Increase task1_current_byte_size 
                current_task_byte_size += length;

                // Dequeue the packet because we have exhausted it.
                queue_try_remove(&slate->radio_packets_out, &payload);
                
                // Check if there is a next payload
                uint8_t next_payload[PAYLOAD_SIZE];
                bool successful_next_peek = queue_try_peek(&slate->radio_packets_out, &next_payload);
                
                // If there is a next payload
                if(successful_next_peek){

                    // Make it the new "payload"
                    memcpy(payload, next_payload, PAYLOAD_SIZE * sizeof(uint8_t));

                    // reset the current_byte_index
                    current_byte_index = 0;
                }
                else{
                    // If there is no next payload, it probably hasn't come yet, so just break
                    // The next for loops will just not do anything cus it won't be able to peek new data.
                    last_place_on_packet = 0;
                    current_byte_index = 0;
                    break;
                }
            }

            else{ // If the function is fully contained in the packet, then do the following:
                
                // COPY ALL OF THE PARTIAL DATA FROM THE PAYLOAD INTO THE STRUCT
                uint8_t* where_to_place_into_struct = &buffer + current_task_byte_size;
                uint8_t* where_to_read_from_payload = &payload + current_byte_index;
                uint8_t length = (sizeof(current_data_holder_task1) - current_task_byte_size);

                // Copy partial data into the struct
                memcpy(where_to_place_into_struct, where_to_read_from_payload, length);
                
                // AT THIS POINT, THE STRUCT SHOULD BE COMPLETED !!! RAHHH !!!
                
                // place the struct into the queue

                // Update the size of task1_current_byte_size
                current_task_byte_size += length;

                // Update the current byte index so that it is now pointing at where the next function should be.
                current_byte_index += length;
            }
        }

        // If the full package was retrieved
        if(current_task_byte_size == max_size_of_struct){
            current_task_byte_size = 0;

            // if the full package was retrieved, and then next thing is a stop, then stop
            if(current_byte_index >= PAYLOAD_SIZE || payload[current_byte_index] == STOP_MNEM ){
                // Delete the packet
                queue_try_remove(&slate->radio_packets_out, &payload);

                // Reset it so the last place on the packet is 0.
                last_place_on_packet = 0;
            }
            else{
                // Don't delete the packet, that would suck.
                // Make sure to save where we left off on this next packet in the queue
                last_place_on_packet = current_byte_index; 
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
    LOG_INFO("dispatching called");

    uint8_t current_byte_index = last_place_on_packet;

    for(int i = 0; i < number_of_commands; i++){
        
        // This payload will store what is currently up next in the radio receive queue
        uint8_t payload[PAYLOAD_SIZE];
        
        // Peek at the upcoming item in the radio receive queue
        bool successful_peek = queue_try_peek(&slate->radio_packets_out, &payload);
        
        if(successful_peek){
            
            // check if the current uploading function number is not  -1,
            // then, set function_id to be whatever it is
            
            // otherwise, try to read the function_id from the payload.

                // Get the function id from the first byte that we are reading from.
                uint8_t function_id = payload[current_byte_index];

            // Increment the byte index forward by the function mnemonic size
            // This means you can start reading data from the next byte
            current_byte_index += FUNC_MNEMONIC_BYTE_SIZE;

            switch(function_id){
                case Func1_MNEM:
                    // How many bytes are in the struct we want? 
                    uint8_t max_size = sizeof(current_data_holder_task1);

                    
                    // While struct only partially filled
                    while(task1_current_byte_size < max_size){

                        // Check how far the struct WOULD go in bytes
                        uint8_t estimated_end_byte_index = current_byte_index + (max_size - task1_current_byte_size);
                        

                        // If the struct would end AFTER the payload size 
                        if(estimated_end_byte_index >= PAYLOAD_SIZE){
                            
                            // Calculate where to read from the packet to place into the struct
                            uint8_t* where_to_place_into_struct = &current_data_holder_task1 + task1_current_byte_size;
                            uint8_t* where_to_read_from_payload = &payload + current_byte_index;
                            uint8_t length = PAYLOAD_SIZE - current_byte_index;


                            // Copy partial data into the struct
                            memcpy(where_to_place_into_struct, where_to_read_from_payload, length);
                            
                            // Increase task1_current_byte_size 
                            task1_current_byte_size += length;

                            // Dequeue the packet because we have exhausted it.
                            queue_try_remove(&slate->radio_packets_out, &payload);
                            
                            // Check if there is a next payload
                            uint8_t next_payload[PAYLOAD_SIZE];
                            bool successful_next_peek = queue_try_peek(&slate->radio_packets_out, &next_payload);
                            
                            // If there is a next payload
                            if(successful_next_peek){

                                // Make it the new "payload"
                                memcpy(payload, next_payload, PAYLOAD_SIZE * sizeof(uint8_t));

                                // reset the current_byte_index
                                current_byte_index = 0;
                            }
                            else{
                                // If there is no next payload, it probably hasn't come yet, so just break
                                // The next for loops will just not do anything cus it won't be able to peek new data.
                                last_place_on_packet = 0;
                                current_byte_index = 0;
                                break;
                            }
                        }

                        else{ // If the function is fully contained in the packet, then do the following:
                            
                            // COPY ALL OF THE PARTIAL DATA FROM THE PAYLOAD INTO THE STRUCT
                            uint8_t* where_to_place_into_struct = &current_data_holder_task1 + task1_current_byte_size;
                            uint8_t* where_to_read_from_payload = &payload + current_byte_index;
                            uint8_t length = (sizeof(current_data_holder_task1) - task1_current_byte_size);

                            // Copy partial data into the struct
                            memcpy(where_to_place_into_struct, where_to_read_from_payload, length);
                            
                            // AT THIS POINT, THE STRUCT SHOULD BE COMPLETED !!! RAHHH !!!
                            
                            // place the struct into the queue

                            // Update the size of task1_current_byte_size
                            task1_current_byte_size += length;

                            // Update the current byte index so that it is now pointing at where the next function should be.
                            current_byte_index += length;
                        }
                    }
                    
                    // AT THIS POINT, AN ENTIRE STRUCT HAS BEEN READ, SO WE WILL ADD IT HERE !
                    queue_try_add(&slate->task1_data, &current_data_holder_task1);

                    // NOW, one of the packets has been read, so we should reset the task1_current_byte_size
                    task1_current_byte_size = 0;

                    // This if statement should only remove packets where the payload took up exactly the size of the payload, or had a stop mnemonic.
                    if(current_byte_index >= PAYLOAD_SIZE || payload[current_byte_index] == STOP_MNEM ){
                        // Delete the packet
                        queue_try_remove(&slate->radio_packets_out, &payload);

                        // Reset it so the last place on the packet is 0.
                        last_place_on_packet = 0;
                    }
                    else{
                        // Don't delete the packet, that would suck.
                        // Make sure to save where we left off on this next packet in the queue
                        last_place_on_packet = current_byte_index; 
                    }
                    break;
                //case Func2_MNEM:
                //    break;
            }


        }



        if (queue_try_remove(&slate->radio_packets_out, &payload))
        { // if successfully dequeued

            // This will keep track of where in the packet we are (each index represents a byte)
            uint8_t current_byte_index = 0;
            
            LOG_INFO("dequeued payload successfully");


            LOG_INFO("payload decoding entered");

            // Get the byte associated with what the function is (0-255)
            uint8_t function_mnemonic = payload[current_byte_index];

            // Go through all the different function options
            switch (function_mnemonic)   // this only works for FUNC_MNEMONIC_BYTE_SIZE = 1 (no need for more?)
            {
                case Func1_MNEM: // TODO test this shit,
                                    // TODO evaluate the need of supporting data of len more than
                                    // the space left in the packet
                    LOG_INFO("Func 1 mnemonic detected");
                    
                    // How much data (including the function mnemonic) is yet to be read
                    uint8_t remaining_size = sizeof(current_data_holder_task1) + FUNC_MNEMONIC_BYTE_SIZE;

                    // potentially an error where if something ISN"T oversized, we aren't checking for the next function???

                    bool searching_for_mnemonic = true;
                    // check if the expected size of a full struct + the function mnenomic (starting from current_byte_index) goes beyond the payload size
                    // this would imply the rest of the data is in some OTHER payload.
                    while (remaining_size > 0){

                        // The address of where to start reading should be from the current byte index
                        uint8_t* address_of_data_start = &payload + current_byte_index;

                        // But if we are currently looking for a mnemonic,
                        if(searching_for_mnemonic){
                            // The start of the actual data should be offset a little
                            address_of_data_start = &payload + current_byte_index + FUNC_MNEMONIC_BYTE_SIZE;

                            // and make sure to reduce the remaining data we are looking for
                            remaining_size -= FUNC_MNEMONIC_BYTE_SIZE;
                        }
                        LOG_INFO("Oversize while loop called");

                        // this is how many bytes until the end of the packet
                        uint8_t length_of_data_until_end_of_packet = PAYLOAD_SIZE - current_byte_index - FUNC_MNEMONIC_BYTE_SIZE;

                        // length = whatever is smaller: bytes remaining to be found, or bytes until the end of this packet
                        uint8_t length = (remaining_size > length_of_data_until_end_of_packet) ? length_of_data_until_end_of_packet : remaining_size;

                        // check how big the data is
                        uint8_t size_of_struct = sizeof(current_data_holder_task1);

                        // address of struct + max size - remaining info - 1 (because that was taken out already)
                        uint8_t* place_in_struct_memory = &current_data_holder_task1 + size_of_struct - remaining_size - 1;

                        // Copy the fraction of the data in the current packet over into the struct.
                        memcpy(place_in_struct_memory, address_of_data_start, length); // copy the rest of the packet

                        // now we know how MUCH data was extracted from this package
                        remaining_size -= length;
                    }

                    break;

                default:
                    printf("Unknown command payload head was: %i", current_byte_index); // should be logged somewhere
                    current_byte_index = STOP_MNEM;  // skip this packet (notify that
                                                // packet was skipped?)
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
