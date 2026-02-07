#include "state_machine.h"

sched_state_t init_state = {.name = "init",
                            .num_tasks = 0,
                            .task_list = {NULL},
                            .get_next_state = NULL};
sched_state_t burn_wire_state = {.name = "burn_wire",
                                 .num_tasks = 0,
                                 .task_list = {NULL},
                                 .get_next_state = NULL};
sched_state_t burn_wire_reset_state = {.name = "burn_wire_reset",
                                       .num_tasks = 0,
                                       .task_list = {NULL},
                                       .get_next_state = NULL};
sched_state_t bringup_state = {.name = "bringup",
                               .num_tasks = 0,
                               .task_list = {NULL},
                               .get_next_state = NULL};
sched_state_t running_state = {.name = "running",
                               .num_tasks = 0,
                               .task_list = {NULL},
                               .get_next_state = NULL};
