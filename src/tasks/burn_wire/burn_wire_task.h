
/**
 * @author  Niklas Vainio
 * @date    2024-08-25
 */

#pragma once

#include "burn_wire.h"
#include "macros.h"
#include "slate.h"
#include "state_machine.h"
#include "typedefs.h"

void burn_wire_task_init(slate_t *slate);
void burn_wire_task_dispatch(slate_t *slate);

extern sched_task_t burn_wire_task;
