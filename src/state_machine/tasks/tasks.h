/**
 * @author  Joseph Shetaye
 * @date    2024-09-30
 *
 * Listing of all tasks. Tasks should not include this file - instead, states
 * should include this file to reference tasks in state definitions.
 */

#pragma once

#include "scheduler/scheduler.h"

/*
 * Tasks are built from the tasks folder, which we cannot depend on to avoid
 * circularity.
 */
extern sched_task_t print_task;
extern sched_task_t blink_task;
extern sched_task_t command_switch_task;
