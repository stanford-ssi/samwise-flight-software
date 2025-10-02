
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

// LED Color for command task - Orange
#define COMMAND_TASK_COLOR 255, 165, 0

void command_task_init(slate_t *slate);
void command_task_dispatch(slate_t *slate);

extern sched_task_t command_task;