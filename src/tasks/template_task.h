/**
 * @author  Niklas Vainio
 * @date    2024-08-27
 *
 * This file is a template to be used for defining your own tasks.
 */

#pragma once

#include "../slate.h"
#include "../state_machine/state_machine.h"

/* TODO - Rename these */
void template_task_init(slate_t *slate);
void template_task_dispatch(slate_t *slate);

sm_task_t template_task;