/**
 * @author  Sasha Luchyn
 * @date    2024-09-11
 */

#pragma once

#include <string.h>

#include "slate.h"
#include "state_machine.h"
#include "typedefs.h"

void command_task_init(slate_t *slate);
void command_task_dispatch(slate_t *slate);

extern sched_task_t command_task;

/*

Here is where we will define all of the structs that hold arguments for calling
certain tasks.

*/
typedef struct
{

} TASK1_DATA;

typedef struct
{
    bool yes_no;
    uint16_t number;
} TASK2_DATA;
