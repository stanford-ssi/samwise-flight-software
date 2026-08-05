#include "ota_state.h"

state_id_t ota_get_next_state(slate_t *slate)
{
    // ota_task clears ota_requested on completion (success or error) and
    // calls rom_reboot on success, so if we reach here the task either
    // hasn't run yet or failed — either way stay in OTA until it finishes.
    if (slate->ota_requested)
        return STATE_OTA;
    return STATE_RUNNING;
}

sched_state_t ota_state = {
    .name = "ota",
    .id = STATE_OTA,
    .num_tasks = 3,
    // radio and watchdog run first so any queued msgs
    // get sent and watchdog gets fed before ota_task
    // blocks for flash ops
    .task_list = {&radio_task, &watchdog_task, &ota_task},
    .get_next_state = &ota_get_next_state};
