/**
 * @author  Darrow Hartman
 * @date    2025-02-15
 *
 * This file abstracts the logging system from the rest of the codebase.
 * It provides a single function for logging messages at different levels to
 * different sinks.
 */
#pragma once

#ifdef TEST_MODE
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#else
#include "pico/printf.h"
#include "pico/stdio.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "pico/types.h"
#endif

// Log levels
typedef enum
{
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO = 1,
    LOG_LEVEL_ERROR = 2
} LOG_LEVEL;

// Output sinks as bit flags
#define LOG_SINK_NONE (0)
#define LOG_SINK_FLASH (1)
#define LOG_SINK_DISK (2)
#define LOG_SINK_TEST (3)
#define LOG_SINK_USB (4)

/* Single main logging function
 * Parameters:
 *  LOG_LEVEL: DEBUG/INFO/ERROR
 *  SINK_BITMASK: 0bxyz (x - FLASH, y - DISK, z - TEST)
 *  fmt: printf-like format string
 *  args: variable args
 */
void log_message(LOG_LEVEL level, uint8_t sink_mask, const char *fmt, ...);

// Convenience macros that automatically handle TEST mode
#ifdef TEST
#define LOG_DEBUG(fmt, ...)                                                    \
    log_message(LOG_LEVEL_DEBUG, LOG_SINK_TEST, "[DEBUG] " fmt "\n",           \
                ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)                                                     \
    log_message(LOG_LEVEL_INFO, LOG_SINK_TEST, "[INFO] " fmt "\n",             \
                ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...)                                                    \
    log_message(LOG_LEVEL_ERROR, LOG_SINK_TEST, "[ERROR] " fmt "\n",           \
                ##__VA_ARGS__)
#else
#define LOG_DEBUG(fmt, ...)                                                    \
    log_message(LOG_LEVEL_DEBUG,                                               \
                LOG_SINK_FLASH | LOG_SINK_DISK | LOG_SINK_USB,                 \
                "[DEBUG] " fmt "\n", ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)                                                     \
    log_message(LOG_LEVEL_INFO, LOG_SINK_FLASH | LOG_SINK_DISK | LOG_SINK_USB, \
                "[INFO] " fmt "\n", ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...)                                                    \
    log_message(LOG_LEVEL_ERROR,                                               \
                LOG_SINK_FLASH | LOG_SINK_DISK | LOG_SINK_USB,                 \
                "[ERROR] " fmt "\n", ##__VA_ARGS__)
#endif

/**
 * Log an error messgae and calls the fatal_error function. In non-flight
 * builds, this locks the system in an unrecoverable panic state
 */
#define ERROR(message)                                                         \
    do                                                                         \
    {                                                                          \
        LOG_ERROR("%s:%d %s\n", __FILE__, __LINE__, message);                  \
        fatal_error(message);                                                  \
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

// Initialize the logger
void logger_init(void);

// Enable/disable specific sinks
void logger_set_sink_enabled(uint8_t sink_mask, bool enabled);
