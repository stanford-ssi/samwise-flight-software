/**
 * @file state_registry.h
 * @brief Central registry mapping state IDs to singleton state structs.
 *
 * The registry provides O(1) lookup from state_id_t to sched_state_t *.
 * States are registered during scheduler initialization.
 */

#pragma once

#include "state_ids.h"
#include "state_machine.h"

/**
 * Register a state in the registry. Called during scheduler initialization.
 */
void state_registry_register(state_id_t id, sched_state_t *state);

/**
 * Look up a state struct by its ID.
 * Returns NULL if the ID is STATE_NONE or invalid.
 */
sched_state_t *state_registry_get(state_id_t id);

/**
 * Get the total number of registered states.
 */
size_t state_registry_count(void);

/**
 * Get state at index i (for iterating over all registered states).
 */
sched_state_t *state_registry_get_by_index(size_t i);
