#include <string.h>
#include <stdio.h>

#include "macros.h"
#include "slate.h"
#include "state_machine.h"
#include "typedefs.h"
#include "stdint.h"

#define FILESYS_TASK_COLOR 255, 192, 203

void filesys_task_init(slate_t *slate);
void filesys_task_dispatch(slate_t *slate);

extern sched_task_t filesys_task;
