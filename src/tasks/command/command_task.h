
/**
 * @author  Thomas Haile
 * @date    2025-05-24
 *
 * Task management for command processing
 */

#pragma once

#include "slate.h"
#include "state_machine.h"
#include "typedefs.h"
#include "rfm9x.h"

void command_task_init(slate_t *slate);
void command_task_dispatch(slate_t *slate);

extern sched_task_t command_task;