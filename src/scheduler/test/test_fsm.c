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
 *
 * The FLIGHT profile additionally covers the burn wire retry loop, driving the
 * panel deploy detect pins via the device status mock.
 */

#include "error.h"
#include "flash.h"
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

// Panel deploy detect state (defined in device_status_mock.c)
extern bool mock_panel_A_deployed;
extern bool mock_panel_B_deployed;

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

#ifdef FLIGHT
    // Nominal deploy: both panels already open, so burn_wire passes straight
    // through. The retry path is covered by tests 4-6.
    mock_panel_A_deployed = true;
    mock_panel_B_deployed = true;
#endif

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

#ifdef FLIGHT
/**
 * Put the FSM back into init with a clean slate, flash counter and panel state.
 */
static void reset_fsm_to_init(void)
{
    memset(&test_slate, 0, sizeof(slate_t));
    test_slate.current_state_id = STATE_INIT;
    test_slate.manual_override_state_id = STATE_NONE;
    test_slate.entered_current_state_time = get_absolute_time();
    test_slate.time_in_current_state_ms = 0;

    // The flash mock's attempt counter is a file static that persists across
    // tests in this binary.
    reset_burn_wire_attempts();
    mock_panel_A_deployed = false;
    mock_panel_B_deployed = false;
    reset_task_stats();
    init_all_tasks(&test_slate);
}

/**
 * Dispatch until the FSM reaches running, or bail out after max_dispatches.
 *
 * Used instead of run_fsm_simulation() because a burn wire retry looks like a
 * stable state to that helper - it re-enters STATE_BURN_WIRE - so the stability
 * heuristic could end the run mid-retry.
 *
 * @return The state the FSM ended in.
 */
static state_id_t run_until_running(uint32_t max_dispatches)
{
    for (uint32_t i = 0; i < max_dispatches; i++)
    {
        mock_time_us += 10 * 1000ULL;
        test_sched_dispatch(&test_slate);

        if (test_slate.current_state_id == STATE_RUNNING)
        {
            break;
        }
    }

    return test_slate.current_state_id;
}

/**
 * Test 4: Both panels already deployed - burn wire is skipped entirely
 */
void test_burn_wire_already_deployed(void)
{
    LOG_DEBUG("=== Test 4: Burn wire, panels already deployed ===");
    log_viz_event("test_start", NULL, "burn_wire_already_deployed");

    reset_fsm_to_init();
    mock_panel_A_deployed = true;
    mock_panel_B_deployed = true;

    ASSERT(run_until_running(10) == STATE_RUNNING);
    ASSERT(get_burn_wire_attempts() == 0);
    ASSERT(test_slate.panel_A_deployed);
    ASSERT(test_slate.panel_B_deployed);

    log_viz_event("test_pass", NULL, "burn_wire_already_deployed");
    LOG_DEBUG("  Test 4 passed");
}

/**
 * Test 5: Neither panel deploys - retry until the attempt budget is spent
 */
void test_burn_wire_retries_until_budget_spent(void)
{
    LOG_DEBUG("=== Test 5: Burn wire retries until budget spent ===");
    log_viz_event("test_start", NULL, "burn_wire_retries");

    reset_fsm_to_init(); // leaves both panels reading closed

    ASSERT(run_until_running(MAX_BURN_WIRE_ATTEMPTS + 5) == STATE_RUNNING);
    ASSERT(get_burn_wire_attempts() == MAX_BURN_WIRE_ATTEMPTS);
    ASSERT(!test_slate.panel_A_deployed);
    ASSERT(!test_slate.panel_B_deployed);

    log_viz_event("test_pass", NULL, "burn_wire_retries");
    LOG_DEBUG("  Test 5 passed");
}

/**
 * Test 6: Only one panel deploys - retry, then hand the asymmetry to the ground
 */
void test_burn_wire_asymmetric_deploy(void)
{
    LOG_DEBUG("=== Test 6: Burn wire asymmetric deploy ===");
    log_viz_event("test_start", NULL, "burn_wire_asymmetric");

    reset_fsm_to_init();
    mock_panel_A_deployed = true;
    mock_panel_B_deployed = false;

    // We still come up in running so the radio works and the ground can see
    // the mismatch in beacon bits 3/4.
    ASSERT(run_until_running(MAX_BURN_WIRE_ATTEMPTS + 5) == STATE_RUNNING);
    ASSERT(get_burn_wire_attempts() == MAX_BURN_WIRE_ATTEMPTS);
    ASSERT(test_slate.panel_A_deployed);
    ASSERT(!test_slate.panel_B_deployed);

    log_viz_event("test_pass", NULL, "burn_wire_asymmetric");
    LOG_DEBUG("  Test 6 passed");
}

/**
 * Test 7: A panel that releases mid-sequence ends the retry loop immediately
 */
void test_burn_wire_deploys_mid_sequence(void)
{
    LOG_DEBUG("=== Test 7: Burn wire deploys mid-sequence ===");
    log_viz_event("test_start", NULL, "burn_wire_mid_sequence");

    reset_fsm_to_init(); // both panels closed

    // Two retries with nothing released...
    mock_time_us += 10 * 1000ULL;
    test_sched_dispatch(&test_slate); // init -> burn_wire

    mock_time_us += 10 * 1000ULL;
    test_sched_dispatch(&test_slate); // burn wire task actually fires
    ASSERT(test_slate.current_state_id == STATE_BURN_WIRE);
    ASSERT(get_burn_wire_attempts() == 1);

    // ...then both panels swing clear.
    mock_panel_A_deployed = true;
    mock_panel_B_deployed = true;

    ASSERT(run_until_running(5) == STATE_RUNNING);
    ASSERT(get_burn_wire_attempts() < MAX_BURN_WIRE_ATTEMPTS);
    ASSERT(test_slate.panel_A_deployed);
    ASSERT(test_slate.panel_B_deployed);

    log_viz_event("test_pass", NULL, "burn_wire_mid_sequence");
    LOG_DEBUG("  Test 7 passed");
}
#endif // FLIGHT

// =============================================================================
// MAIN
// =============================================================================

int main(void)
{
    LOG_DEBUG("=== FSM Integration Test (profile: %s) ===", get_profile_name());

    char *profile_name = get_profile_name();
    char basename[8 + strlen(profile_name) +
                  6]; // "fsm_viz_" + profile_name + ".json\0"
    snprintf(basename, sizeof(basename), "fsm_viz_%s.json", profile_name);

    viz_log_open_log_dir(basename);

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
#ifdef FLIGHT
    test_burn_wire_already_deployed();
    test_burn_wire_retries_until_budget_spent();
    test_burn_wire_asymmetric_deploy();
    test_burn_wire_deploys_mid_sequence();
#endif

    LOG_DEBUG("=== All FSM Tests Passed (profile: %s) ===", get_profile_name());
    LOG_DEBUG("Total simulated time: %lu ms",
              (unsigned long)(mock_time_us / 1000));

    log_viz_event("fsm_end", NULL, profile_details);
    viz_log_close();

    return 0;
}
