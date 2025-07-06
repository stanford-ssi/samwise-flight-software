
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

// LED Color for burn_wire task - Bright White
#define BURN_WIRE_TASK_COLOR 255, 255, 255

void burn_wire_task_init(slate_t *slate);
void burn_wire_task_dispatch(slate_t *slate);

extern sched_task_t burn_wire_task;
