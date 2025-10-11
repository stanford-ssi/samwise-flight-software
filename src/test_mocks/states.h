#pragma once

// Mock states.h for tests
// Note: We declare the state externals but don't include their headers
// since those may have dependencies not available in test builds

#include "state_machine.h"

// State declarations - actual definitions are in driver_stubs.c
extern sched_state_t init_state;
extern sched_state_t burn_wire_state;
extern sched_state_t burn_wire_reset_state;
extern sched_state_t bringup_state;
extern sched_state_t running_state;
