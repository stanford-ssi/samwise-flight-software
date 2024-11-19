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
#include "drivers/rfm9x.h"

typedef struct samwise_slate
{
    /*
     * State machine info.
     */
    sched_state_t *current_state;
    absolute_time_t entered_current_state_time;
    uint64_t time_in_current_state_ms;

    bool led_state;

    /*
     * Radio
     */
    rfm9x_t radio;
    queue_t tx_queue;
    queue_t rx_queue;
} slate_t;
