
/**
 * @author  Niklas Vainio
 * @date    2024-08-25
 */

#pragma once

#include "slate.h"
#include "scheduler/scheduler.h"

void print_task_init(slate_t *slate);
void print_task_dispatch(slate_t *slate);

sched_task_t print_task;