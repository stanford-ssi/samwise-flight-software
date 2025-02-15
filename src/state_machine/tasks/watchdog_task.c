/**
 * @author  <Chloe Zhong>
 * @date    <1/13/2025>
 *
 * <watchdog program which feeds watchdog with a pulse every 20 seconds. Pulse lasts for 10 microseconds.>
 */


#include "watchdog_task.h"
#include "macros.h"
#include "slate.h"
#include "pico/stdlib.h"


int WATCHDOG_PIN = 6;

/*
 * IMPORTANT: Remember to add your task to all_tasks in state_machine_states.h
 */

void watchdog_task_init(slate_t *slate)
{
    
    gpio_init(WATCHDOG_PIN);
    gpio_set_dir(WATCHDOG_PIN, GPIO_OUT); //sets resting pin state to low
}

void watchdog_task_dispatch(slate_t *slate)
{
    gpio_put(WATCHDOG_PIN, true); //puts pin on high
    sleep_us(10); //hold pin on high for 10 microseconds
    gpio_put(WATCHDOG_PIN, false); //returns pin to low
}

sched_task_t template_task = {.name = "watchdog",
                           .dispatch_period_ms = 20000, //Ethan said to feed watchdog every 20ish seconds
                           .task_init = &watchdog_task_init,   
                           .task_dispatch = &watchdog_task_dispatch,

                           .next_dispatch = 0};

