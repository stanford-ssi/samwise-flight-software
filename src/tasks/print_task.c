
/**
 * @author  Niklas Vainio
 * @date    2024-08-25
 */

#include "print_task.h"
#include "../macros.h"
#include "../slate.h"

void print_task_init(slate_t *slate)
{
    LOG_INFO("Test task is initializing...");
}

void print_task_dispatch(slate_t *slate)
{
    LOG_INFO("Test task is dispatching...");
}

sm_task_t print_task = {.name = "print",
                        .dispatch_period_ms = 1000,
                        .task_init = &print_task_init,
                        .task_dispatch = &print_task_dispatch,

                        /* Set to an actual value on init */
                        .next_dispatch = 0};