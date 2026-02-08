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
#include "state_registry.h"
#include "typedefs.h"

void sched_init(slate_t *slate);
void sched_dispatch(slate_t *slate);
