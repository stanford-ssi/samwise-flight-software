/**
 * @author  Sasha Luchyn
 * @date    2024-09-11
 */

#pragma once

#include "scheduler/scheduler.h"
#include "slate.h"

void command_switch_task_init(slate_t *slate);
void command_switch_dispatch(slate_t *slate);

// sched_task_t command_switch;  // Not a good idea to comment out?
