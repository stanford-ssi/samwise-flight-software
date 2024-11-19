/**
 * @author  Sasha Luchyn
 * @date    2024-09-11
 *
 * Task to parse incoming bitstream and allocate data for according tasks
 * This is a very rough sketch of how it should work, nothing works
 */

// Function Mnemonics FuncName assignedNumber
// assignedNumber < (2^(FUNC_MNEMONIC_BYTE_SIZE)*8) -1
#define STOP_MNEM 255
#define Func1_MNEM 1
//.....
#define Func254_MNEM 254

//

#include "state_machine/tasks/command_switch_task.h"
#include "macros.h"
#include "pico/stdlib.h"
#include "slate.h"

#define PAYLOAD_SIZE 251                     // in bytes TODO check?
const int RADIO_PACKETS_OUT_MAX_LENGTH = 32; // max queue length
const int FUNC_MNEMONIC_BYTE_SIZE = 1;
// const int PACKET_DATA_LENGTH_VAR_MIN_LENGTH =
//     1; // minimum size of the part of data that shows the length of input
//     data
//        // to the previous function
const int TASK1_QUEUE_LENGTH = 32; // max queue length for task 1
struct TASK1_DATA_STRUCT_FORMAT
{
    int data_int_1;
    char data_byteArr_1[16];
};

unsigned char payload[PAYLOAD_SIZE];
// unsigned char data_length = 0;
unsigned char payload_head = 0;
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

/// struc (int a, char c, int b)
/// aaaa cxxx bbbb
///
///

void command_switch_dispatch(slate_t *slate)
{
    if (queue_try_remove(&slate->radio_packets_out, &payload))
    { // if successfully dequeued

        while (payload[payload_head] <= 255 &&
               payload[payload_head] != STOP_MNEM) // if not end of packet data
        {
            switch (payload[payload_head])   // this only works for FUNC_MNEMONIC_BYTE_SIZE = 1
            {
                case Func1_MNEM: // TODO test this shit,
                                 // TODO add support for data of len more than
                                 // the space left in the packet

                    // copy to struct for the first function sizeof(struct) bytes
                    // from payload array starting at payload_head + 1
                    memcpy(&T1DS, &payload + payload_head + FUNC_MNEMONIC_BYTE_SIZE, sizeof(T1DS));
                    queue_try_add(&slate->task1_data, &T1DS);       // adding the task data to the appropriate queue
                    payload_head += FUNC_MNEMONIC_BYTE_SIZE;        // move for func mnem 
                    payload_head += sizeof(T1DS);                   // move for data size 
                    break;

                default:
                    printf("Unknown command"); // should be logged somewhere
                    payload_head = STOP_MNEM;  // skip this packet (notify that
                                               // packet was skipped?)
                    break;
            }
        }
    }


    // OLD questions
    // go over each (one?) packet and handle accordingly
    // 1) determine function thats called
    // 2) determine input data length (if datalen==max possible num to
    // represent, check next PACKET_DATA_LENGTH_VAR_MIN_LENGTH of data until
    // full data length obtained)
    // 3) put the data into queue that correlates to the function we determined
    // earlier (Another task will check if queue is empty, if not empty function
    // for that task will be called)
    // empty explored packets from queue

    // TODO: / Questions to ask:
    // Can the radio driver return variable sized packets or is it always
    // constant?
    // What datasize should be the queue initialized to?
    // What length should the queue be initialized to?
    // Should we work in Bytes or go down to bits?
}

sched_task_t command_switch_task = {.name = "command_switch",
                                    .dispatch_period_ms = 100,
                                    .task_init = &command_switch_task_init,
                                    .task_dispatch = &command_switch_dispatch,
                                    /* Set to an actual value on init */
                                    .next_dispatch = 0};
