/*
 * @author Marc Aaron Reyes
 * @ February 10, 2025
 *
 * This task is used to test the camera of the Payload module.
 */

#pragma once 

#include "scheduler/scheduler.h"
#include "slate.h"

void take_photo_task_init(slate_t *slate);
void take_photo_task_dispatch(slate_t *slate);

sched_task_t take_photo_task;
