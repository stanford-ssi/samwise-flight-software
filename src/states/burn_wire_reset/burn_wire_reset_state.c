#include "burn_wire_reset_state.h"
#include "flash.h"
#include "logger.h"

extern sched_state_t burn_wire_reset_state;

state_id_t burn_wire_reset_get_next_state(slate_t *slate)
{
    reset_burn_wire_attempts(); // Reset burn wire attempts
    LOG_INFO("Burn wire reset state: resetting burn wire attempts");
    return STATE_RUNNING;
}

sched_state_t burn_wire_reset_state = {.name = "burn_wire_reset",
                                       .id = STATE_BURN_WIRE_RESET,
                                       .num_tasks = 0,
                                       .task_list = {},
                                       .get_next_state =
                                           &burn_wire_reset_get_next_state};
