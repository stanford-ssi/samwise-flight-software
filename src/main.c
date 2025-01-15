/**
 * @author  Niklas Vainio
 * @date    2024-08-23
 *
 * This file contains the main entry point for the SAMWISE flight code.
 */

#include "init.h"
#include "macros.h"
#include "pico/stdlib.h"
#include "scheduler/scheduler.h"
#include "slate.h"

#include "state_machine/tasks/command_switch_task.h"

/**
 * Statically allocate the slate.
 */
slate_t slate;

/**
 * Main code entry point.
 *
 * This should never return (unless something really bad happens!)
 */
int main()
{   
    // Some ugly code with linter errors
    int x = 10 + 5;
    stdio_init_all();
    
    /*
     * In debug builds, delay to allow the user to connect to open the serial
     * port.
     */
    if (!IS_FLIGHT)
    {
        sleep_ms(5000);
    }

    /*
     * Initialize everything.
     */
    LOG_INFO("main: Slate uses %d bytes", sizeof(slate));
    LOG_INFO("main: Initializing everything...");
    ASSERT(init(&slate));
    LOG_INFO("main: Initialized successfully!\n\n\n");

    /*
     * Go state machine!
     */
    LOG_INFO("main: Dispatching the state machine...");
    // while (true)
    // {
    //     sched_dispatch(&slate);
    // }
    

    /* TESTS 2 function calls in one packet */
    struct test_t1DS{
        int data_int_1;
        uint8_t data_byteArr_1[16];
    };
    struct test_t1DS first;
    struct test_t1DS second;

    struct test_t1DS first_out;
    struct test_t1DS second_out;

    first.data_int_1 = 229;
    first.data_byteArr_1[0] = 1;
    first.data_byteArr_1[1] = 2;
    first.data_byteArr_1[2] = 3;
    first.data_byteArr_1[15] = 101;

    second.data_int_1 = 303;
    second.data_byteArr_1[0] = 111;
    second.data_byteArr_1[9] = 222;
    second.data_byteArr_1[15] = 101;


    uint8_t packet1[251];

    packet1[0] = 1;

    LOG_INFO("PLEASE I SWEAR TO FUCKING OD, size is: %i", sizeof(first));

    memcpy(&packet1[1], &first, sizeof(first));

    LOG_INFO("MEOWWWWW");

    *(packet1 + sizeof(first) + 1 * sizeof(uint8_t)) = 1;

    memcpy(packet1 + sizeof(first) + 2 * sizeof(uint8_t), &second, sizeof(second));

    *(packet1 + 2 * sizeof(first) + 2) = 255;

    LOG_INFO("Packet before sending:");

    for (int i = 0; i < 2 * sizeof(first) + 5; i++){
        LOG_INFO("Byte At %i is %i", i, packet1[i]);
    }

    command_switch_task_init(&slate);  // init queue

    // meow
    queue_try_add(&slate.radio_packets_out, &packet1);

    LOG_INFO("Radio Payload added to queue");

    command_switch_dispatch(&slate);
    command_switch_dispatch(&slate);

    // get the struct
    queue_try_remove(&slate.task1_data, &first_out);
    queue_try_remove(&slate.task1_data, &second_out);

    LOG_INFO("Expected integer 1: %i, received: %i", first.data_int_1, first_out.data_int_1);

    for(int i =0 ; i < 16; i++){
        LOG_INFO("Packet 1, Index %i: Expected: %i, Received %i", i, first.data_byteArr_1[i], first_out.data_byteArr_1[i]);
    }
    LOG_INFO("Expected integer 2: %i, received: %i", second.data_int_1, second.data_int_1);

    for(int i =0 ; i < 16; i++){
        LOG_INFO("Packet 2, Index %i: Expected: %i, Received %i", i, second.data_byteArr_1[i], second_out.data_byteArr_1[i]);
    }

    /* End of TESTS */

    /*
     * We should NEVER be here so something bad has happened.
     * @todo reboot!
     */
    ERROR("We reached the end of the code - this is REALLY BAD!");
}
