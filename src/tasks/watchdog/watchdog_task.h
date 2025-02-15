/**
 * @author  <Chloe Zhong>
 * @date    <1/11/2025>
 *
 * <watchdog header file>
 */

#pragma once

#include "slate.h"
#include "state_machine/states/states.h"

void watchdog_task_init(slate_t *slate);
void watchdog_task_dispatch(slate_t *slate);

sched_task_t watchdog_task;
