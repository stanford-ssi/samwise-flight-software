/**
 * @author  Sasha Luchyn
 * @date    2024-09-11
 *
 * Task to parse incoming bitstream and allocate data for according tasks
 * This is a very rough sketch of how it should work, nothing works
 */

// Function Mnemonics FuncName assignedNumber
// assignedNumber < 2^(FUNC_MNEMONIC_BYTE_SIZE)
#define Func1 0
//.....
#define Func127 127

#include "state_machine/tasks/command_switch_task.h"
#include "macros.h"
#include "pico/stdlib.h"
#include "slate.h"
const int RADIO_PACKETS_OUT_MAX_LENGTH = 32; // max queue length
const int PAYLOAD_SIZE = 251;                // in bytes
const int FUNC_MNEMONIC_BYTE_SIZE = 1;
const int PACKET_DATA_LENGTH_VAR_MIN_LENGTH =
    1; // minimum size of the part of data that shows the length of input data
       // to the previous function

void command_switch_task_init(slate_t *slate)
{
    // this would be done differently and in the radio receive task or something
    // of its kind, not here
    char *packet = (char *)malloc(PAYLOAD_SIZE * sizeof(char));
    ASSERT(packet != NULL);
    queue_init(&slate->radio_packets_out, sizeof(packet),
               RADIO_PACKETS_OUT_MAX_LENGTH);
}

void command_switch_dispatch(slate_t *slate)
{
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
