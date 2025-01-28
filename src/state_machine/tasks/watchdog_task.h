/**
 * @author  <Chloe Zhong>
 * @date    <1/11/2025>
 *
 * <BRIEF DESCRIPTION OF TASK>
 */

#pragma once

#include "slate.h"
#include "state_machine/states/states.h"

void watchdog_task_init(slate_t *slate);
void watchdog_task_dispatch(slate_t *slate);

sm_task_t watchdog_task;
