#include "shutdown_state.h"
#include "logger.h"

state_id_t shutdown_get_next_state(slate_t *slate)
{
    (void)slate;
    return STATE_SHUTDOWN;
}

sched_state_t shutdown_state = {
    .name = "shutdown",
    .id = STATE_SHUTDOWN,
    .num_tasks = 1,
    .task_list = {&watchdog_task},
    .get_next_state = &shutdown_get_next_state};
