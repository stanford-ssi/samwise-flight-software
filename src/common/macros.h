/**
 * @author  Niklas Vainio
 * @date    2024-08-25
 *
 * This file defines several convenience macros for logging, assertions, and
 * error handling.
 */

#pragma once

#include "error.h"
#if TEST == 1
#include <stdio.h>
#else
#include "pico/printf.h"
#endif

/**
 * If this symbol is defined, we are configured to run a flight build.
 *
 * All flight/non-flight-secific behavior should check this symbol, or the
 * convenience macros below (#ifdef FLIGHT... / if (IS_FLIGHT)...)
 *
 * IMPORTANT: Uncomment before flight!
 */
// #define FLIGHT

/**
 * Convenience macros to get whether we are in flight in a runtime build.
 */
#ifdef FLIGHT
#define IS_FLIGHT true
#else
#define IS_FLIGHT false
#endif

/**
 * Log a formatted message at the debug level. Will only do anything in a
 * non-flight build.
 */
#ifndef FLIGHT
#define LOG_DEBUG(fmt, ...)                                                    \
    printf("[DEBUG]   " fmt "\n" __VA_OPT__(, ) __VA_ARGS__)
#else
#define LOG_DEBUG(fmt, ...) (void)0
#endif

/**
 * Log a printf-style formatted message at the info level. Will log in both
 * flight and test builds.
 */
#define LOG_INFO(fmt, ...)                                                     \
    printf("[INFO]    " fmt "\n" __VA_OPT__(, ) __VA_ARGS__)

/**
 * Log a printf-style formatted error message. Will log in both flight and test
 * builds.
 */
#define LOG_ERROR(fmt, ...)                                                    \
    printf("[ERROR]   " fmt "\n" __VA_OPT__(, ) __VA_ARGS__)

/**
 * Log an error messgae and calls the fatal_error function. In non-flight
 * builds, this locks the system in an unrecoverable panic state
 */
#define ERROR(message)                                                         \
    do                                                                         \
    {                                                                          \
        LOG_ERROR("%s:%d %s\n", __FILE__, __LINE__, message);                  \
        fatal_error();                                                         \
    } while (0)

/**
 * Assert a certain condition at runtime and raise an error if it is false.
 */
#define ASSERT(condition)                                                      \
    do                                                                         \
    {                                                                          \
        if (!(condition))                                                      \
        {                                                                      \
            ERROR("Assertion failed: " #condition);                            \
        }                                                                      \
    } while (0)

/**
 * Assert a certain condition only in debug builds.
 */
#ifdef FLIGHT
#define DEBUG_ASSERT(condition) (void)0
#else
#define DEBUG_ASSERT(condition) ASSERT(condition, message)
#endif
