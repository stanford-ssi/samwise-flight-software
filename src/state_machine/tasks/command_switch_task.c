/**
 * @author  Sasha Luchyn
 * @date    2024-09-11
 *
 * Task to parse incoming bitstream and allocate data for according tasks
 * This is a very rough sketch of how it should work, nothing works
 */

// Function Mnemonics FuncName assignedNumber
// assignedNumber < (2^(FUNC_MNEMONIC_BYTE_SIZE)*8) -1
#define STOP_MNEM 0
#define Func1_MNEM 1
//.....
#define Func127_MNEM 255

//

#include "state_machine/tasks/command_switch_task.h"
#include "macros.h"
#include "pico/stdlib.h"
#include "slate.h"

#define PAYLOAD_SIZE 251                     // in bytes
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

// void byte_to_obj(const unsigned *byteArr, void *object,
//                  size_t size) // change to function for eacch specfii
// {
//     for (int i = 0; i < size; i++)
//     {
//         object += byteArr[i];
//     }
// }
// void read_bytes(char *byteArrIn, void *data, unsigned char len,
//                 unsigned char start)
// {
//     for (int i = 0; i < len; i++)
//     {
//         data += byteArrIn[i];
//     }
// }

void sereailize_struct(unsigned char Func_MNEM, char *byteArr,
                       void *data_struct)
{
    switch (Func_MNEM)
    {
        case Func1_MNEM:
            // read_bytes(&byteArr, &T1DS.data_int_1, sizeof(T1DS.data_int_1),
            // 0);

            // read_bytes(&byteArr, &T1DS.data_byteArr_1,
            //            sizeof(T1DS.data_byteArr_1), sizeof(T1DS.data_int_1));

            // T1DS.data_int_1 = byteArr[0] + byteArr[1] + byteArr[2] +
            // byteArr[3]; T1DS.data_byteArr_1 =

            break;

        default:
            break;
    }
}

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
            switch (payload[payload_head])
            {
                case Func1_MNEM: // TODO test this shit,
                                 // TODO add support for data of len more than
                                 // the space left in the packet
                    // byte_to_obj(&payload[payload_head], &T1DS, sizeof(T1DS));
                    // queue_try_add(&slate->task1_data, &T1DS);
                    // payload_head += sizeof(T1DS);
                    // break;

                default:
                    printf("Unknown command"); // should be logged somewhere
                    payload_head = STOP_MNEM;  // skip this packet (notify that
                                               // packet was skipped?)
                    break;
            }
        }
    }

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
