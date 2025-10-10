#pragma once

#include <stdbool.h>
#include <stdint.h>

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

void log_message(LOG_LEVEL level, uint8_t sink_mask, const char *fmt, ...);

#define LOG_DEBUG(fmt, ...)                                                    \
    log_message(LOG_LEVEL_DEBUG, LOG_SINK_TEST, "[DEBUG] " fmt "\n",           \
                ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)                                                     \
    log_message(LOG_LEVEL_INFO, LOG_SINK_TEST, "[INFO] " fmt "\n",             \
                ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...)                                                    \
    log_message(LOG_LEVEL_ERROR, LOG_SINK_TEST, "[ERROR] " fmt "\n",           \
                ##__VA_ARGS__)

#define ERROR(message)                                                         \
    do                                                                         \
    {                                                                          \
        LOG_ERROR("%s:%d %s\n", __FILE__, __LINE__, message);                  \
        fatal_error();                                                         \
    } while (0)

#define ASSERT(condition)                                                      \
    do                                                                         \
    {                                                                          \
        if (!(condition))                                                      \
        {                                                                      \
            ERROR("Assertion failed: " #condition);                            \
        }                                                                      \
    } while (0)

void logger_init(void);
void logger_set_sink_enabled(uint8_t sink_mask, bool enabled);
