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
#define STOP_MNEM 255
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
struct TASK1_DATA_STRUCT_FORMAT
{
    int data_int_1;
    uint8_t data_byteArr_1[16];
};

uint8_t payload[PAYLOAD_SIZE];
// unsigned char data_length = 0;
uint8_t payload_head = 0;
struct TASK1_DATA_STRUCT_FORMAT T1DS;


void command_switch_task_init(slate_t *slate)
{
    // initialize queue for radio input data (TODO: potentially move to radio
    // stuff)
    queue_init(&slate->radio_packets_out, sizeof(payload),
               RADIO_PACKETS_OUT_MAX_LENGTH);

    // initialize queue for task 1
    queue_init(&slate->task1_data, sizeof(T1DS), TASK1_QUEUE_LENGTH);
}

struct TASK1_DATA_STRUCT_FORMAT* T1DS_count;  // TODO deal with this shit

void command_switch_dispatch(slate_t *slate)
{
    LOG_INFO("dispatching called");
    if (queue_try_remove(&slate->radio_packets_out, &payload))
    { // if successfully dequeued
    payload_head = 0;
    LOG_INFO("dequeued payload successfully");
    

        while (payload_head < PAYLOAD_SIZE &&
               payload_head != STOP_MNEM && payload[payload_head] != STOP_MNEM) // clean up
        {
            LOG_INFO("payload decoding entered");
            switch (payload[payload_head])   // this only works for FUNC_MNEMONIC_BYTE_SIZE = 1 (no need for more?)
            {
                case Func1_MNEM: // TODO test this shit,
                                 // TODO evaluate the need of supporting data of len more than
                                 // the space left in the packet
                    LOG_INFO("Func 1 mnemonic detected");
                    T1DS_count = &T1DS;
                    // support for data longer than the space left in a packet (potentially useless?)
                    while (payload_head + FUNC_MNEMONIC_BYTE_SIZE + sizeof(T1DS) >= PAYLOAD_SIZE ){
                        LOG_INFO("Oversize while loop called");
                        // copy max length, modify payload head and try to dequeue a new payload
                        memcpy(&T1DS_count, &payload + payload_head + FUNC_MNEMONIC_BYTE_SIZE,
                                     PAYLOAD_SIZE - payload_head - FUNC_MNEMONIC_BYTE_SIZE); // copy the rest of the packet
                        T1DS_count += PAYLOAD_SIZE - payload_head - FUNC_MNEMONIC_BYTE_SIZE;    // move pointer of the DS
                        if (!queue_try_remove(&slate->radio_packets_out, &payload)){    // get next packet
                            // show message that there was a misalignment with the data?
                            payload_head = STOP_MNEM;
                            break;
                        }
                        payload_head = 0;                                       // start next packet from start
                    }

                    // copy to struct for the first function sizeof(struct) bytes
                    // from payload array starting at payload_head + 1
                    for(int i = 0; i < sizeof(payload); i++){
                        LOG_DEBUG("Payload: %i", payload[i]);
                    }
                    LOG_DEBUG("T1DS size %i vs payload size %i ", sizeof(T1DS), sizeof(payload));

                    memcpy(&T1DS, &payload[payload_head + FUNC_MNEMONIC_BYTE_SIZE], sizeof(T1DS)); //% PAYLOAD_SIZE);
                    queue_try_add(&slate->task1_data, &T1DS);       // adding the task data to the appropriate queue
                    /// test            
                    LOG_INFO("added to func 1 arr int: %i and by arr char 14 is: %i", T1DS.data_int_1, T1DS.data_byteArr_1[14]);
                    /// test
                    payload_head += FUNC_MNEMONIC_BYTE_SIZE + 1;        // move for func mnem and look next item
                    payload_head += sizeof(T1DS) % PAYLOAD_SIZE;                   // move for data size 
                    LOG_INFO("next payload funcMnenm %i ", payload[payload_head]);
                    break;

                default:
                    printf("Unknown command payload head was: %i", payload_head); // should be logged somewhere
                    payload_head = STOP_MNEM;  // skip this packet (notify that
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
