/**
 * @author  Niklas Vainio
 * @date    2024-08-25
 *
 * This file defines the types and global declarations for the satellite state
 * machine.
 */

#pragma once

#include "pico/time.h"
#include "pico/types.h"

#include "macros.h"
#include "slate.h"
#include "state_machine.h"
#include "typedefs.h"

// Add required include files for states
#include "init_state.h"
#include "running_state.h"

/*
 * Must be a macro because it is used to initialize an array
 */
#define num_states (sizeof(all_states) / sizeof(sched_state_t *))

void sched_init(slate_t *slate);
void sched_dispatch(slate_t *slate);
