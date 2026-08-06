#pragma once

#include <stdint.h>

typedef struct samwise_slate slate_t;
typedef struct sched_state sched_state_t;
typedef struct sched_task sched_task_t;
typedef struct _rfm9x rfm9x_t;
typedef struct onboard_led onboard_led_t;
typedef struct watchdog watchdog_t;

/*
 * Platform type portability: in production builds, pull pico-sdk types.
 * In test/host builds, provide equivalent definitions.
 */
#ifndef TEST
#include "pico/time.h"
#else
typedef uint64_t absolute_time_t;
typedef unsigned int uint;
typedef struct
{
    // Data stored in the queue, i.e. a generic byte buffer.
    uint8_t *data;

    // The size of each element in the queue (in bytes).
    unsigned int element_size;

    // The capacity of the queue (in number of elements).
    unsigned int element_count;

    // The index of the head of the queue (where elements are removed from).
    unsigned int head;

    // The index of the tail of the queue (where elements are added to).
    unsigned int tail;

    // The current number of elements in the queue.
    unsigned int level;
} queue_t;
#endif
