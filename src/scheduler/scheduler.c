/**
 * @author  Niklas Vainio
 * @date    2024-08-25
 *
 * This file defines the tasks and states of the state machine, used for
 * scheduling and dispatching tasks on the satellite.
 */

#include "scheduler.h"
#include "state_registry.h"
#include "error.h"
#include "logger.h"

#include "init_state.h"
#include "running_state.h"
#include "burn_wire_state.h"
#include "burn_wire_reset_state.h"
#ifdef BRINGUP
#include "bringup_state.h"
#endif

static size_t n_tasks = 0;
static sched_task_t *all_tasks[STATE_COUNT * MAX_TASKS_PER_STATE];

/**
 * Initialize the state machine.
 */
void sched_init(slate_t *slate)
{
    // Register all states
    state_registry_register(STATE_INIT, &init_state);
    state_registry_register(STATE_RUNNING, &running_state);
    state_registry_register(STATE_BURN_WIRE, &burn_wire_state);
    state_registry_register(STATE_BURN_WIRE_RESET, &burn_wire_reset_state);
#ifdef BRINGUP
    state_registry_register(STATE_BRINGUP, &bringup_state);
#endif

    size_t num_states = state_registry_count();

    /*
     * Check that each state has a valid number of tasks, and enumerate all
     * tasks.
     */
    for (size_t i = 0; i < num_states; i++)
    {
        sched_state_t *state = state_registry_get_by_index(i);
        ASSERT(state->num_tasks <= MAX_TASKS_PER_STATE);
        for (size_t j = 0; j < state->num_tasks; j++)
        {
            bool is_duplicate = 0;
            for (size_t k = 0; k < n_tasks; k++)
            {
                if (all_tasks[k] == state->task_list[j])
                    is_duplicate = 1;
            }
            if (!is_duplicate)
            {
                all_tasks[n_tasks] = state->task_list[j];
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
    slate->current_state_id = STATE_INIT;
    slate->manual_override_state_id = STATE_NONE;
    slate->entered_current_state_time = get_absolute_time();
    slate->time_in_current_state_ms = 0;

    LOG_DEBUG("sched: Done initializing!");
}

/**
 * Dispatch the state machine. Runs any of the current state's tasks which are
 * due, and transitions into the next state.
 */
void sched_dispatch(slate_t *slate)
{
    sched_state_t *current_state_info =
        state_registry_get(slate->current_state_id);

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
    state_id_t next_state_id;
    if (slate->manual_override_state_id != STATE_NONE)
    {
        sched_state_t *override =
            state_registry_get(slate->manual_override_state_id);
        LOG_INFO("sched: Manual state override to %s", override->name);
        next_state_id = slate->manual_override_state_id;
        slate->manual_override_state_id = STATE_NONE;
    }
    else
    {
        next_state_id = current_state_info->get_next_state(slate);
    }

    if (next_state_id != slate->current_state_id)
    {
        sched_state_t *next = state_registry_get(next_state_id);
        LOG_DEBUG("sched: Transitioning to state %s", next->name);

        slate->current_state_id = next_state_id;
        slate->entered_current_state_time = get_absolute_time();
        slate->time_in_current_state_ms = 0;
    }
}
