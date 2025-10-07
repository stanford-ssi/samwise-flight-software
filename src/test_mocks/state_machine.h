#pragma once

#include <stdlib.h>
#include <stdint.h>
#include "slate.h"

#define MAX_TASKS_PER_STATE 10

typedef struct sched_task
{
    const char *name;
    const uint32_t dispatch_period_ms;
    absolute_time_t next_dispatch;
    void (*task_init)(slate_t *slate);
    void (*task_dispatch)(slate_t *slate);
} sched_task_t;

typedef struct sched_state
{
    const char *name;
    size_t num_tasks;
    sched_task_t *task_list[MAX_TASKS_PER_STATE];
    struct sched_state *(*get_next_state)(slate_t *slate);
} sched_state_t;
