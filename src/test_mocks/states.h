#pragma once

// Mock states.h for tests  
// In tests, we don't need to include all the state headers
// Just provide the state_machine types

#include "state_machine.h"

// Forward declarations for states that might be referenced
extern sched_state_t running_state;
extern sched_state_t init_state;
extern sched_state_t bringup_state;
extern sched_state_t burn_wire_state;
extern sched_state_t burn_wire_reset_state;
