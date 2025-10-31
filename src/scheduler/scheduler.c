/**
 * @author  Niklas Vainio
 * @date    2024-08-25
 *
 * This file defines the tasks and states of the state machine, used for
 * scheduling and dispatching tasks on the satellite.
 */

#include "scheduler.h"
#include "states.h"

/*
 * Include the actual state machine
 */
static const sched_state_t *all_states[] = {&init_state,
#ifdef BRINGUP
                                            &bringup_state,
#endif
                                            &running_state};
static sched_state_t *const initial_state = &init_state;
static size_t n_tasks = 0;
static sched_task_t *all_tasks[num_states * MAX_TASKS_PER_STATE];

/**
 * Initialize the state machine.
 *
 * @param slate     Pointer to the slate.
 */
void sched_init(slate_t *slate)
{
    /*
     * Check that each state has a valid number of tasks, and enumerate all
     * tasks.
     */
    LOG_DEBUG("Initializing tasks for %d states", num_states);
    for (size_t i = 0; i < num_states; i++)
    {
        LOG_DEBUG(". %s", all_states[i]->name);
        ASSERT(all_states[i]->num_tasks <= MAX_TASKS_PER_STATE);
        for (size_t j = 0; j < all_states[i]->num_tasks; j++)
        {
            bool is_duplicate = 0;
            for (size_t k = 0; k < n_tasks; k++)
            {
                if (all_tasks[k] == all_states[i]->task_list[j])
                    is_duplicate = 1;
            }
            if (!is_duplicate)
            {
                all_tasks[n_tasks] = all_states[i]->task_list[j];
                n_tasks++;
            }
        }
    }

    LOG_DEBUG("sched: Enumerated %d tasks", n_tasks);

    /*
     * Initialize all tasks.
     */
    for (size_t i = 0; i < n_tasks; i++)
    {
        LOG_DEBUG("sched: Initializing task %s", all_tasks[i]->name);
        all_tasks[i]->task_init(slate);
    }

    for (size_t i = 0; i < n_tasks; i++)
    {
        all_tasks[i]->next_dispatch =
            make_timeout_time_ms(all_tasks[i]->dispatch_period_ms);
    }

    /*
     * Enter the init state by default
     */
    slate->current_state = initial_state;
    slate->entered_current_state_time = get_absolute_time();
    slate->time_in_current_state_ms = 0;

    LOG_DEBUG("sched: Done initializing!");
}

/**
 * Dispatch the state machine. Runs any of the current state's tasks which are
 * due, and transitions into the next state.
 *
 * @param slate     Pointer to the slate.
 */
void sched_dispatch(slate_t *slate)
{
    sched_state_t *current_state_info = slate->current_state;

    /*
     * Loop through all of this state's tasks
     */
    for (size_t i = 0; i < current_state_info->num_tasks; i++)
    {
        sched_task_t *task = current_state_info->task_list[i];

        /*
         * Check if this task is due and if so, dispatch it
         */
        if (absolute_time_diff_us(task->next_dispatch, get_absolute_time()) > 0)
        {
            task->next_dispatch =
                make_timeout_time_ms(task->dispatch_period_ms);

            task->task_dispatch(slate);
        }
    }

    slate->time_in_current_state_ms =
        absolute_time_diff_us(slate->entered_current_state_time,
                              get_absolute_time()) /
        1000;

    /*
     * Transition to the next state, if required.
     */
    sched_state_t *next_state;
    if (slate->manual_override_state != NULL)
    {
        LOG_INFO("sched: Manual state override to %s",
                 slate->manual_override_state->name);
        next_state = slate->manual_override_state;
        slate->manual_override_state = NULL;
    }
    else
    {
        next_state = current_state_info->get_next_state(slate);
    }

    if (next_state != current_state_info)
    {
        LOG_DEBUG("sched: Transitioning to state %s", next_state->name);

        slate->current_state = next_state;
        slate->entered_current_state_time = get_absolute_time();
        slate->time_in_current_state_ms = 0;
    }
}
