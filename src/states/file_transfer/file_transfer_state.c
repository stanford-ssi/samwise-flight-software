#include "file_transfer_state.h"

extern sched_state_t file_transfer_state;

sched_state_t *file_transfer_get_next_state(slate_t *slate)
{
    return &file_transfer_state;
}

sched_state_t file_transfer_state = {
    .name = "file_transfer",
    .num_tasks = 5,
    .task_list = {&watchdog_task, &file_transfer_task, &radio_task,
                  &command_task, &beacon_task},
    .get_next_state = &file_transfer_get_next_state};
