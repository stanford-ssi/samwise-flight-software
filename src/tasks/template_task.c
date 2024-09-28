/**
 * @author  Niklas Vainio
 * @date    2024-08-27
 *
 * This file is a template to be used for defining your own tasks.
 */

#include "template_task.h"
#include "../macros.h"
#include "../slate.h"

/*
 * IMPORTANT: Remember to add your task to all_tasks in state_machine_states.h
 */

void template_task_init(slate_t *slate)
{
    /* TODO */
}

void template_task_dispatch(slate_t *slate)
{
    /* TODO */
}

sm_task_t template_task = {.name = "TODO",
                           .dispatch_period_ms = 99999,              /* TODO */
                           .task_init = &template_task_init,         /* TODO*/
                           .task_dispatch = &template_task_dispatch, /* TODO */

                           /* Set to an actual value on init */
                           .next_dispatch = 0};