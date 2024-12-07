#include "macros.h"

#ifdef BRINGUP

#include "scan_task.h"
#include "slate.h"

void scan_task_init(slate_t *slate)
{
    LOG_INFO("Test task is initializing...");
}

void scan_task_dispatch(slate_t *slate)
{
    LOG_INFO("Test task is dispatching...");
}

sched_task_t scan_task = {.name = "scan",
                          .dispatch_period_ms = 1000,
                          .task_init = &scan_task_init,
                          .task_dispatch = &scan_task_dispatch,

                          /* Set to an actual value on init */
                          .next_dispatch = 0};

#endif
