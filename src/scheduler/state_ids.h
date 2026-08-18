/**
 * @file state_ids.h
 * @brief State identifier enum for the SAMWISE state machine.
 *
 * This header has ZERO dependencies and can be safely included by any file
 * without creating circular includes. States reference each other by ID
 * instead of by extern pointer, breaking compile-time circular dependencies.
 */

#pragma once

typedef enum
{
    STATE_NONE = -1, // Sentinel: no state (replaces NULL pointer)
    STATE_INIT = 0,  // Must be 0 for slate initialization
    STATE_RUNNING,
    STATE_BURN_WIRE,
    STATE_BURN_WIRE_RESET,
    STATE_BRINGUP,
    STATE_SHUTDOWN,
    STATE_COUNT // Must be last: total number of states
} state_id_t;
