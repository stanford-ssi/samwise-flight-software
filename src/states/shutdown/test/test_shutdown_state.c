/**
 * @file test_shutdown_state.c
 * @brief Unit tests for the shutdown state's structure and exit logic.
 *
 * Covers the review feedback on PR #313:
 *   - Shutdown must exit to STATE_RUNNING (not STATE_INIT, which re-runs burn
 *     wire on FLIGHT).
 *   - Ground reactivation clears the shutdown flag and leaves shutdown.
 *   - The auto-timeout is only a fallback safety net.
 *   - The persistent shutdown flag is cleared whenever we leave shutdown.
 */

#include "flash.h"
#include "logger.h"
#include "shutdown_state.h"
#include "slate.h"
#include "state_ids.h"
#include "test_scheduler_helpers.h"
#include <stdio.h>
#include <string.h>

slate_t test_slate;

// Kept in sync with SHUTDOWN_TIMEOUT_MS in shutdown_state.c (3 months).
#define SHUTDOWN_TIMEOUT_MS (90ULL * 24 * 60 * 60 * 1000)

/**
 * Test 1: Verify shutdown state structure.
 */
void test_shutdown_state_structure()
{
    LOG_DEBUG("=== Test 1: Shutdown state structure ===");

    ASSERT(shutdown_state.name != NULL);
    ASSERT(strcmp(shutdown_state.name, "shutdown") == 0);
    ASSERT(shutdown_state.id == STATE_SHUTDOWN);
    ASSERT(shutdown_state.num_tasks == 2);
    ASSERT(shutdown_state.get_next_state != NULL);

    // The state must run the watchdog (so it can be fed) and the receive-only
    // reactivation listener.
    bool found_watchdog = false;
    bool found_listen = false;
    for (size_t i = 0; i < shutdown_state.num_tasks; i++)
    {
        sched_task_t *task = shutdown_state.task_list[i];
        ASSERT(task != NULL);
        ASSERT(task->name != NULL);
        ASSERT(task->task_init != NULL);
        ASSERT(task->task_dispatch != NULL);
        ASSERT(task->dispatch_period_ms > 0);

        if (strcmp(task->name, "watchdog") == 0)
            found_watchdog = true;
        if (strcmp(task->name, "shutdown_listen") == 0)
            found_listen = true;
    }
    ASSERT(found_watchdog);
    ASSERT(found_listen);

    LOG_DEBUG("✓ Test 1 passed");
}

/**
 * Test 2: While shutdown is active and the fallback has not elapsed, we stay
 * in shutdown.
 */
void test_stays_in_shutdown()
{
    LOG_DEBUG("=== Test 2: Stays in shutdown ===");

    ASSERT(clear_and_init_slate(&test_slate) == 0);
    set_shutdown_active();
    test_slate.shutdown_triggered = true;
    test_slate.time_in_current_state_ms = 1000; // well under the fallback

    ASSERT(shutdown_state.get_next_state(&test_slate) == STATE_SHUTDOWN);
    // Still shut down, so the persistent flag must remain set.
    ASSERT(get_shutdown_active() != 0);

    free_slate(&test_slate);
    LOG_DEBUG("✓ Test 2 passed");
}

/**
 * Test 3: Ground reactivation (shutdown_triggered cleared by the listener)
 * exits to STATE_RUNNING, NOT STATE_INIT.
 */
void test_ground_reactivation_exits_to_running()
{
    LOG_DEBUG("=== Test 3: Ground reactivation exits to running ===");

    ASSERT(clear_and_init_slate(&test_slate) == 0);
    // Simulate the listener having authorized reactivation: flag cleared.
    test_slate.shutdown_triggered = false;
    test_slate.time_in_current_state_ms = 1000;

    state_id_t next = shutdown_state.get_next_state(&test_slate);
    ASSERT(next == STATE_RUNNING);
    ASSERT(next != STATE_INIT);

    free_slate(&test_slate);
    LOG_DEBUG("✓ Test 3 passed");
}

/**
 * Test 4: The fallback timeout auto-reactivates, clears the persistent flag,
 * and exits to STATE_RUNNING.
 */
void test_fallback_timeout()
{
    LOG_DEBUG("=== Test 4: Fallback timeout ===");

    ASSERT(clear_and_init_slate(&test_slate) == 0);
    set_shutdown_active();
    test_slate.shutdown_triggered = true;
    test_slate.shutdown_cmd_counter = 3;
    test_slate.shutdown_reactivate_counter = 1;
    test_slate.time_in_current_state_ms = SHUTDOWN_TIMEOUT_MS;

    state_id_t next = shutdown_state.get_next_state(&test_slate);
    ASSERT(next == STATE_RUNNING);
    ASSERT(test_slate.shutdown_triggered == false);
    ASSERT(test_slate.shutdown_cmd_counter == 0);
    ASSERT(test_slate.shutdown_reactivate_counter == 0);
    // Persistent flag must be cleared so we do not re-enter shutdown on reboot.
    ASSERT(get_shutdown_active() == 0);

    free_slate(&test_slate);
    LOG_DEBUG("✓ Test 4 passed");
}

int main()
{
    LOG_DEBUG("=== Shutdown State Tests ===");

    test_shutdown_state_structure();
    test_stays_in_shutdown();
    test_ground_reactivation_exits_to_running();
    test_fallback_timeout();

    LOG_DEBUG("=== All Shutdown State Tests Passed ===");
    return 0;
}
