/**
 * @author  Niklas Vainio
 * @date    2024-08-25
 *
 * This file defines the tasks and states of the state machine, used for
 * scheduling and dispatching tasks on the satellite.
 */

#include "state_machine.h"
#include "../macros.h"
#include "../slate.h"
#include "pico/time.h"
#include "state_machine_states.h"

/**
 * Initialize the state machine.
 *
 * @param slate     Pointer to the slate.
 */
void sm_init(slate_t *slate)
{
    /*
     * Check that each state has a valid number of tasks.
     */
    for (size_t i = 0; i < num_states; i++)
    {
        ASSERT(all_states[i].num_tasks <= MAX_TASKS_PER_STATE);
    }

    /*
     * Initialize all tasks.
     */
    for (size_t i = 0; i < NUM_TASKS; i++)
    {
        LOG_INFO("sm: Initializing task %s", all_tasks[i]->name);
        all_tasks[i]->task_init(slate);
    }

    for (size_t i = 0; i < NUM_TASKS; i++)
    {
        all_tasks[i]->next_dispatch =
            make_timeout_time_ms(all_tasks[i]->dispatch_period_ms);
    }

    /*
     * Enter the init state by default
     */
    slate->current_state = state_init;
    slate->entered_current_state_time = get_absolute_time();
    slate->time_in_current_state_ms = 0;

    LOG_DEBUG("sm: Done initializing!");
}

/**
 * Dispatch the state machine. Runs any of the current state's tasks which are
 * due, and transitions into the next state.
 *
 * @param slate     Pointer to the slate.
 */
void sm_dispatch(slate_t *slate)
{
    sm_state_t current_state = slate->current_state;
    sm_state_info_t *current_state_info = all_states + slate->current_state;

    /*
     * Loop through all of this state's tasks
     */
    for (size_t i = 0; i < current_state_info->num_tasks; i++)
    {
        sm_task_t *task = current_state_info->task_list[i];

        /*
         * Check if this task is due and if so, dispatch it
         */
        if (absolute_time_diff_us(task->next_dispatch, get_absolute_time()) > 0)
        {
            task->next_dispatch =
                make_timeout_time_ms(task->dispatch_period_ms);

            LOG_DEBUG("sm: Dispatching task %s", task->name);
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
    const sm_state_t next_state = current_state_info->get_next_state(slate);
    if (next_state != current_state)
    {
        LOG_INFO("sm: Transitioning to state %s", all_states[next_state].name);

        slate->current_state = next_state;
        slate->entered_current_state_time = get_absolute_time();
        slate->time_in_current_state_ms = 0;
    }
}
