#include "burn_wire_state.h"
#include "flash.h"

extern sched_state_t burn_wire_reset_state;

sched_state_t *burn_wire_reset_get_next_state(slate_t *slate)
{
    reset_burn_wire_attempts(); // Reset burn wire attempts
    LOG_INFO("Burn wire reset state: resetting burn wire attempts");
    return &running_state;
}

sched_state_t burn_wire_reset_state = {.name = "burn_wire_reset",
                                       .num_tasks = 0,
                                       .task_list = {},
                                       .get_next_state =
                                           &burn_wire_reset_get_next_state};
