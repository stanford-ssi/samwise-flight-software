#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "logger.h"
#include "slate.h"

// Helper macro for test assertions with error messages
#define TEST_ASSERT(cond, msg, ...)                                            \
    do                                                                         \
    {                                                                          \
        if (!(cond))                                                           \
        {                                                                      \
            LOG_ERROR("TEST ASSERTION FAILED at %s:%d - " msg, __FILE__,       \
                      __LINE__, ##__VA_ARGS__);                                \
            return -1;                                                         \
        }                                                                      \
    } while (0)

/**
 * Function pointer type for initializing a slate before running a test. This
 * allows tests to specify custom slate initialization logic if needed, while
 * still providing a default initialization function (clear_and_init_slate) for
 * convenience.
 */
typedef int (*test_harness_init_slate_func_t)(slate_t *slate);

/**
 * Function pointer type for individual test cases. Each test case function
 * takes a pointer to a slate and returns 0 on success or a negative value on
 * failure.
 */
typedef int (*test_harness_func_t)(slate_t *slate);

/**
 * Struct representing a single test case, including its unique ID, name, the
 * test function to execute, and whether it requires slate initialization before
 * running.
 */
typedef struct test_harness_case
{
    uint16_t test_id; // Unique identifier for the test case, useful for
                      // selective test execution
    test_harness_func_t test_func;
    const char *name;
} test_harness_case_t;

/**
 * Runs a suite of tests and logs the results. Returns 0 if all tests passed, or
 * -1 if any test failed.
 */
int test_harness_run(const char *suite_name, const test_harness_case_t *tests,
                     size_t num_tests,
                     const test_harness_init_slate_func_t init_func);

/**
 * Runs a subset of tests specified by the id array. Returns 0 if all
 * selected tests passed, or -1 if any test failed. Also will return -1 if
 * unable to construct the subset of tests (e.g. id not found).
 */
int test_harness_include_run(const char *suite_name,
                             const test_harness_case_t *cases, size_t num_tests,
                             const test_harness_init_slate_func_t init_func,
                             const uint16_t *ids, size_t num_ids);

/**
 * Runs all tests except for the ones specified by the exclude_ids array.
 * Returns 0 if all selected tests passed, or -1 if any test failed.
 */
int test_harness_exclude_run(const char *suite_name,
                             const test_harness_case_t *cases, size_t num_tests,
                             const test_harness_init_slate_func_t init_func,
                             const uint16_t *exclude_ids,
                             size_t num_exclude_ids);
