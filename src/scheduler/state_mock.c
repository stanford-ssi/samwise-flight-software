#include "state_ids.h"
#include "state_machine.h"

static state_id_t mock_init_next(slate_t *s)
{
    return STATE_INIT;
}
static state_id_t mock_burn_wire_next(slate_t *s)
{
    return STATE_BURN_WIRE;
}
static state_id_t mock_burn_wire_reset_next(slate_t *s)
{
    return STATE_BURN_WIRE_RESET;
}
static state_id_t mock_bringup_next(slate_t *s)
{
    return STATE_BRINGUP;
}
static state_id_t mock_running_next(slate_t *s)
{
    return STATE_RUNNING;
}

sched_state_t init_state = {.id = STATE_INIT,
                            .name = "init",
                            .num_tasks = 0,
                            .task_list = {NULL},
                            .get_next_state = mock_init_next};
sched_state_t burn_wire_state = {.id = STATE_BURN_WIRE,
                                 .name = "burn_wire",
                                 .num_tasks = 0,
                                 .task_list = {NULL},
                                 .get_next_state = mock_burn_wire_next};
sched_state_t burn_wire_reset_state = {.id = STATE_BURN_WIRE_RESET,
                                       .name = "burn_wire_reset",
                                       .num_tasks = 0,
                                       .task_list = {NULL},
                                       .get_next_state =
                                           mock_burn_wire_reset_next};
sched_state_t bringup_state = {.id = STATE_BRINGUP,
                               .name = "bringup",
                               .num_tasks = 0,
                               .task_list = {NULL},
                               .get_next_state = mock_bringup_next};
sched_state_t running_state = {.id = STATE_RUNNING,
                               .name = "running",
                               .num_tasks = 0,
                               .task_list = {NULL},
                               .get_next_state = mock_running_next};
