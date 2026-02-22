/**
 * @file hardware_test_assert.h
 * @brief Non-fatal ASSERT override for BRINGUP hardware tests
 *
 * ──────────────────────────────────────────────────────────────────────
 *  HOW THIS WORKS
 * ──────────────────────────────────────────────────────────────────────
 *
 *  The normal ASSERT macro (from logger.h) calls fatal_error() on
 *  failure, which panics/halts the MCU.  During BRINGUP we want to
 *  run hardware tests without crashing — a failed assertion should
 *  log the error and keep going.
 *
 *  This header #undefs the stock ASSERT and replaces it with one that
 *  only logs the failure via LOG_ERROR.  Execution continues after
 *  the failed assertion.
 *
 *  Usage:
 *    Include this AFTER logger.h (and any other headers that pull in
 *    the stock ASSERT) so that it overrides the macro:
 *
 *        #include "logger.h"
 *        #include "mram.h"
 *        ...
 *        #include "hardware_test_assert.h"   // must be LAST include
 *
 *    Nothing else needs to change — existing ASSERT() calls will use
 *    the non-fatal version in BRINGUP builds and the normal
 *    fatal_error() version in all other builds.
 * ──────────────────────────────────────────────────────────────────────
 */

#pragma once

#ifdef BRINGUP

/* Replace the stock ASSERT with a non-fatal version.
 * On failure: log the error and continue execution. */
#undef ASSERT
#define ASSERT(condition)                                                      \
    do                                                                         \
    {                                                                          \
        if (!(condition))                                                      \
        {                                                                      \
            LOG_ERROR("ASSERT FAILED: \"%s\" at %s:%d", #condition, __FILE__,  \
                      __LINE__);                                               \
        }                                                                      \
    } while (0)

#endif /* BRINGUP */
