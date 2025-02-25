/**
 * @author  <Chloe Zhong>
 * @date    <1/13/2025>
 *
 * <watchdog program which feeds watchdog with a pulse>
 */


#include "watchdog_task.h"

void watchdog_task_init(slate_t *slate)
{}

void watchdog_task_dispatch(slate_t *slate)
{
    watchdog_feed(&slate->watchdog);
}

sched_task_t watchdog_task = {.name = "watchdog",
                           .dispatch_period_ms = 100, //Ethan said to feed watchdog every 20ish seconds
                           .task_init = &watchdog_task_init,   
                           .task_dispatch = &watchdog_task_dispatch,

                           .next_dispatch = 0};

