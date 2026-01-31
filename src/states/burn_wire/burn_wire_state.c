#include "burn_wire_state.h"
#include "states.h"
#include "flash.h"

extern sched_state_t burn_wire_state;

sched_state_t *burn_wire_get_next_state(slate_t *slate)
{
    // Need to check if burn wire task has successfully completed
    // otherwise, stay in burn wire state and retry.
    // TODO: need to check status on the satellite side panel pins
    //       for early exit from burn_wire re-attempts.

    // Check if burn wire is already activated
    uint32_t burn_wire_attempts = get_burn_wire_attempts();
    if (burn_wire_attempts >= MAX_BURN_WIRE_ATTEMPTS)
    {
        LOG_ERROR(
            "Maximum burn wire attempts reached. Transitioning to running state.");
        return &running_state;
    }
    else
    {
        LOG_INFO("Burn wire attempts below maximum. Retrying burn wire.");
        return &burn_wire_state;
    }
}

sched_state_t burn_wire_state = {.name = "burn_wire",
                                 .num_tasks = 1,
                                 .task_list = {&burn_wire_task},
                                 .get_next_state = &burn_wire_get_next_state};
