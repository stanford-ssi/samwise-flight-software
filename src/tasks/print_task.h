
/**
 * @author  Niklas Vainio
 * @date    2024-08-25
 */

#pragma once

#include "../slate.h"
#include "../state_machine/state_machine.h"

void print_task_init(slate_t *slate);
void print_task_dispatch(slate_t *slate);

sm_task_t print_task;