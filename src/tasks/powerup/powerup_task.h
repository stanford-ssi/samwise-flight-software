/**
 * @author  Marc Aaron Reyes
 * @date    2025-05-05
 */

#pragma once

#include "macros.h"
#include "scheduler.h"
#include "slate.h"

void powerup_task_init(slate_t *slate);
void powerup_task_dispatch(slate_t *slate);

extern sched_task_t powerup_task;
