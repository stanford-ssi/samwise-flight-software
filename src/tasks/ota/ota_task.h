#pragma once

#include "slate.h"
#include "state_machine.h"

void ota_task_init(slate_t *slate);
void ota_task_dispatch(slate_t *slate);

extern sched_task_t ota_task;
