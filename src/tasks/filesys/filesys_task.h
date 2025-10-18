/**
 * @author  Claude Code
 * @date    2025-10-18
 *
 * File system task for MRAM testing - writes and reads random data
 */

#pragma once

#include "macros.h"
#include "slate.h"
#include "state_machine.h"
#include "typedefs.h"

// LED Color for filesys task - Pink
#define FILESYS_TASK_COLOR 255, 20, 147

void filesys_task_init(slate_t *slate);
void filesys_task_dispatch(slate_t *slate);

extern sched_task_t filesys_task;