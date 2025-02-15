/**
 * @author  Darrow Hartman
 * @date    2025-02-15
 *
 * This file abstracts the logging system from the rest of the codebase.
 * It provides a single function for logging messages at different levels to
 * different sinks.
 */
#pragma once

#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
// Log levels
typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO = 1,
    LOG_LEVEL_ERROR = 2
} LOG_LEVEL;

// Output sinks as bit flags
#define LOG_SINK_NONE  0b000
#define LOG_SINK_FLASH 0b100
#define LOG_SINK_DISK  0b010
#define LOG_SINK_TEST  0b001

/* Single main logging function
 * Parameters:
 *  LOG_LEVEL: DEBUG/INFO/ERROR
 *  SINK_BITMASK: 0bxyz (x - FLASH, y - DISK, z - TEST)
 *  fmt: printf-like format string
 *  args: variable args
 */
void log_message(LOG_LEVEL level, uint8_t sink_mask, const char* fmt, ...);



// Convenience macros that automatically handle TEST mode
#ifdef TEST
    #define LOG_DEBUG(fmt, ...) log_message(LOG_LEVEL_DEBUG, LOG_SINK_TEST, "[DEBUG] " fmt "\n", ##__VA_ARGS__)
    #define LOG_INFO(fmt, ...)  log_message(LOG_LEVEL_INFO, LOG_SINK_TEST, "[INFO] " fmt "\n", ##__VA_ARGS__)
    #define LOG_ERROR(fmt, ...) log_message(LOG_LEVEL_ERROR, LOG_SINK_TEST, "[ERROR] " fmt "\n", ##__VA_ARGS__)
#else
    #define LOG_DEBUG(fmt, ...) log_message(LOG_LEVEL_DEBUG, LOG_SINK_FLASH | LOG_SINK_DISK, "[DEBUG] " fmt "\n", ##__VA_ARGS__)
    #define LOG_INFO(fmt, ...)  log_message(LOG_LEVEL_INFO, LOG_SINK_FLASH | LOG_SINK_DISK, "[INFO] " fmt "\n", ##__VA_ARGS__)
    #define LOG_ERROR(fmt, ...) log_message(LOG_LEVEL_ERROR, LOG_SINK_FLASH | LOG_SINK_DISK, "[ERROR] " fmt "\n", ##__VA_ARGS__)
#endif

// Initialize the logger
void logger_init(void);

// Enable/disable specific sinks
void logger_set_sink_enabled(uint8_t sink_mask, bool enabled);
