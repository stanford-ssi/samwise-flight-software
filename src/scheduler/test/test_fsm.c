/**
 * @file test_fsm.c
 * @brief FSM integration test - exercises full state machine transitions
 *
 * Tests the complete state machine for a given build profile, starting from
 * STATE_INIT and running until the FSM reaches a stable state (same state
 * returned for 5 consecutive dispatch cycles).
 *
 * Build profiles tested via separate Bazel targets:
 *   - fsm_test_flight  (FLIGHT defined)  : init -> burn_wire -> running
 *   - fsm_test_bringup (BRINGUP defined) : init -> bringup
 *   - fsm_test_debug   (no special defs) : init -> running
 */

#include "error.h"
#include "logger.h"
#include "pico/stdlib.h"
#include "state_ids.h"
#include "state_registry.h"
#include "test_scheduler_helpers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// State headers
#include "burn_wire_reset_state.h"
#include "burn_wire_state.h"
#include "init_state.h"
#include "running_state.h"
#ifdef BRINGUP
#include "bringup_state.h"
#endif

slate_t test_slate;

// Stable count threshold: stop when state unchanged for this many dispatches
#define STABLE_THRESHOLD 5

// =============================================================================
// HELPERS
// =============================================================================

/**
 * Register all states relevant to the current build profile.
 * Mirrors the registration in scheduler.c sched_init().
 */
static void register_all_states(void)
{
    state_registry_register(STATE_INIT, &init_state);
    state_registry_register(STATE_RUNNING, &running_state);
    state_registry_register(STATE_BURN_WIRE, &burn_wire_state);
    state_registry_register(STATE_BURN_WIRE_RESET, &burn_wire_reset_state);
#ifdef BRINGUP
    state_registry_register(STATE_BRINGUP, &bringup_state);
#endif
}

/**
 * Initialize all unique tasks across all registered states.
 */
static void init_all_tasks(slate_t *slate)
{
    // Track which tasks we've already initialized (by pointer)
    sched_task_t *initialized[STATE_COUNT * MAX_TASKS_PER_STATE];
    size_t n_initialized = 0;

    size_t num_states = state_registry_count();
    for (size_t i = 0; i < num_states; i++)
    {
        sched_state_t *state = state_registry_get_by_index(i);
        for (size_t j = 0; j < state->num_tasks; j++)
        {
            sched_task_t *task = state->task_list[j];

            // Check if already initialized
            bool already = false;
            for (size_t k = 0; k < n_initialized; k++)
            {
                if (initialized[k] == task)
                {
                    already = true;
                    break;
                }
            }

            if (!already)
            {
                task->task_init(slate);
                task->next_dispatch =
                    make_timeout_time_ms(task->dispatch_period_ms);
                log_viz_event("task_init", task->name, "initialized");
                initialized[n_initialized++] = task;
            }
        }
    }

    LOG_DEBUG("Initialized %zu unique tasks across %zu states", n_initialized,
              num_states);
}

static const char *get_profile_name(void)
{
#ifdef FLIGHT
    return "picubed-flight";
#elif defined(BRINGUP)
    return "picubed-bringup";
#elif defined(PICO)
    return "pico";
#else
    return "picubed-debug";
#endif
}

// =============================================================================
// TESTS
// =============================================================================

/**
 * Test 1: Verify all states are registered and valid
 */
void test_state_registration(void)
{
    LOG_DEBUG("=== Test 1: State registration ===");
    log_viz_event("test_start", NULL, "state_registration");

    size_t num_states = state_registry_count();
    ASSERT(num_states > 0);
    LOG_DEBUG("  Registered %zu states for profile %s", num_states,
              get_profile_name());

    for (size_t i = 0; i < num_states; i++)
    {
        sched_state_t *state = state_registry_get_by_index(i);
        ASSERT(state != NULL);
        ASSERT(state->name != NULL);
        ASSERT(state->get_next_state != NULL);
        LOG_DEBUG("  State %zu: %s (id=%d, tasks=%zu)", i, state->name,
                  state->id, state->num_tasks);
    }

    // Verify init state is always registered
    ASSERT(state_registry_get(STATE_INIT) != NULL);
    // Verify running state is always registered
    ASSERT(state_registry_get(STATE_RUNNING) != NULL);

#ifdef BRINGUP
    ASSERT(state_registry_get(STATE_BRINGUP) != NULL);
#endif

    log_viz_event("test_pass", NULL, "state_registration");
    LOG_DEBUG("  Test 1 passed");
}

/**
 * Test 2: Run full FSM simulation from init until stable
 */
void test_fsm_transitions(void)
{
    LOG_DEBUG("=== Test 2: FSM transitions (profile: %s) ===",
              get_profile_name());
    log_viz_event("test_start", NULL, "fsm_transitions");

    // Run FSM: 10ms dispatch interval, log every 1s, stop after 5 stable
    state_id_t final_state =
        run_fsm_simulation(&test_slate, 10, 1000, STABLE_THRESHOLD);

    // Verify expected final stable state per profile
#ifdef BRINGUP
    ASSERT(final_state == STATE_BRINGUP);
    LOG_DEBUG("  BRINGUP profile: stabilized in bringup state");
#else
    ASSERT(final_state == STATE_RUNNING);
    LOG_DEBUG("  Profile %s: stabilized in running state", get_profile_name());
#endif

    log_viz_event("test_pass", NULL, "fsm_transitions");
    LOG_DEBUG("  Test 2 passed");
}

/**
 * Test 3: Run scheduler in the stable state for 10 seconds to verify tasks
 */
void test_stable_state_execution(void)
{
    LOG_DEBUG("=== Test 3: Stable state execution ===");
    log_viz_event("test_start", NULL, "stable_state_execution");

    sched_state_t *stable_state =
        state_registry_get(test_slate.current_state_id);
    ASSERT(stable_state != NULL);
    LOG_DEBUG("  Running in stable state: %s (%zu tasks)", stable_state->name,
              stable_state->num_tasks);

    // Simulate 10 seconds in the stable state
    run_scheduler_simulation(&test_slate, 10000, 10, 2000);

    // Verify state didn't change during stable execution
    ASSERT(test_slate.current_state_id == stable_state->id);

    LOG_DEBUG("  Stable state execution completed successfully");
    log_viz_event("test_pass", NULL, "stable_state_execution");
    LOG_DEBUG("  Test 3 passed");
}

// =============================================================================
// MAIN
// =============================================================================

int main(void)
{
    LOG_DEBUG("=== FSM Integration Test (profile: %s) ===", get_profile_name());

    // Build output filename, writing to TEST_UNDECLARED_OUTPUTS_DIR if
    // available so Bazel preserves the JSON after the test run.
    char filename[256];
    const char *out_dir = getenv("TEST_UNDECLARED_OUTPUTS_DIR");
    char basename[64];
    snprintf(basename, sizeof(basename), "fsm_viz_%s.json", get_profile_name());
    // Replace hyphens with underscores for valid filenames
    for (char *p = basename; *p; p++)
    {
        if (*p == '-')
            *p = '_';
    }
    if (out_dir != NULL)
    {
        snprintf(filename, sizeof(filename), "%s/%s", out_dir, basename);
    }
    else
    {
        snprintf(filename, sizeof(filename), "%s", basename);
    }
    viz_log_open(filename);

    // Log profile info
    char profile_details[64];
    snprintf(profile_details, sizeof(profile_details), "profile=%s",
             get_profile_name());
    log_viz_event("fsm_start", NULL, profile_details);

    mock_time_us = 0;
    memset(&test_slate, 0, sizeof(slate_t));
    reset_task_stats();

    // Setup initial state
    test_slate.current_state_id = STATE_INIT;
    test_slate.manual_override_state_id = STATE_NONE;
    test_slate.entered_current_state_time = get_absolute_time();
    test_slate.time_in_current_state_ms = 0;

    // Register all states and initialize tasks
    register_all_states();
    init_all_tasks(&test_slate);

    // Run tests
    test_state_registration();
    test_fsm_transitions();
    test_stable_state_execution();

    LOG_DEBUG("=== All FSM Tests Passed (profile: %s) ===", get_profile_name());
    LOG_DEBUG("Total simulated time: %lu ms",
              (unsigned long)(mock_time_us / 1000));

    log_viz_event("fsm_end", NULL, profile_details);
    viz_log_close();

    return 0;
}
