/**
 * @author  Niklas Vainio
 * @date    2024-08-25
 *
 * This file defines the slate struct, a static struct which stores all data on
 * the satellite. At init time, a single instance of this struct gets statically
 * allocated, and it is referenced by all tasks and functions.
 *
 * Look up "blackboard pattern" for more info.
 */

#pragma once

#include "pico/types.h"
#include "pico/util/queue.h"
#include "scheduler/scheduler.h"

#define max_datastructure_size 304 // buffer size should be maxsize of biggest datastructure

typedef struct samwise_slate
{
    /*
     * State machine info.
     */
    sched_state_t *current_state;
    absolute_time_t entered_current_state_time;
    uint64_t time_in_current_state_ms;

    bool led_state;

    queue_t rx_queue;  // assuming this will be a queue of packets
    
    // command_switch_task data below
    queue_t task1_data; // queues of this kind will exist for each task called
                        // from radio com
    queue_t task2_data;

    uint8_t buffer[max_datastructure_size];    

    uint16_t current_task_byte_size;
    uint16_t current_byte_index;
    uint16_t last_place_on_packet;
    uint8_t uploading_function_number;

} slate_t;
