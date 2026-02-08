/**
 * @file hardware_tests.h
 * @brief Hardware test registry for BRINGUP builds
 *
 * This header declares all hardware test entry points and provides a static
 * table that hardware_test_task uses to automatically run every registered
 * test during init.
 *
 * ──────────────────────────────────────────────────────────────────────
 *  HOW TO ADD A NEW HARDWARE TEST
 * ──────────────────────────────────────────────────────────────────────
 *  1.  Create your test file in  src/tasks/hardware_test/test/<name>.c
 *      with the normal test pattern (functions + main).
 *  2.  Guard main() so it gets a unique name in BRINGUP builds:
 *
 *          #ifdef BRINGUP
 *          void <name>_main(void)
 *          #else
 *          int main(void)
 *          #endif
 *
 *  3.  Declare the BRINGUP entry point below (step A).
 *  4.  Add it to the HW_TEST_TABLE macro          (step B).
 *  5.  Add the .c file to the BRINGUP sources in
 *      hardware_test/CMakeLists.txt and link any needed drivers.
 * ──────────────────────────────────────────────────────────────────────
 */

#pragma once

#include <stddef.h>

/* ── Test function signature ─────────────────────────────────────────── */

typedef void (*hw_test_fn)(void);

typedef struct
{
    const char *name;
    hw_test_fn run;
} hw_test_entry_t;

/* ── Step A: declare each test's BRINGUP entry point ─────────────────── */

void mram_test_main(void);

/* ── Step B: register tests — add one line per test ─────────────────── */

#define HW_TEST_TABLE                                                          \
    {                                                                          \
        {"mram_test", mram_test_main},                                         \
    }
