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

    // command_switch_task data below
    queue_t
        radio_packets_out; // My guess at what input data queue will look like
    queue_t task1_data; // queues of this kind will exist for each task called
                        // from radio com
    queue_t task2_data;

    uint8_t struct_buffer[max_datastructure_size];

    uint16_t num_uploaded_bytes;
    uint16_t packet_buffer_index;
    uint16_t last_place_on_packet;
    uint8_t uploading_command_id;

} slate_t;
