/**
 * @author  <Chloe Zhong>
 * @date    <1/11/2025>
 *
 * <watchdog header file>
 */

#pragma once

#include "slate.h"
#include "macros.h"
#include "pico/stdlib.h"

void watchdog_task_init(slate_t *slate);
void watchdog_task_dispatch(slate_t *slate);

extern sched_task_t watchdog_task;
