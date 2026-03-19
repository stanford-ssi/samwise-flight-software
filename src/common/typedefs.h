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
#if !defined(TEST) && !defined(PICO)
#include "pico/time.h"
#else
typedef uint64_t absolute_time_t;
typedef unsigned int uint;
typedef struct
{
    uint8_t *data;
    unsigned int element_size;
    unsigned int element_count;
    unsigned int head;
    unsigned int tail;
    unsigned int level;
} queue_t;
#endif
