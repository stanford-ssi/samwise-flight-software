#include "logger.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// External reference to current executing task (from test_scheduler_helpers.c)
extern const char *current_executing_task;
extern void log_viz_task_message(const char *task_name, const char *log_message);

void log_message(LOG_LEVEL level, uint8_t sink_mask, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    // If we have a current executing task, capture the log to viz log
    if (current_executing_task != NULL)
    {
        char buffer[512];
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        log_viz_task_message(current_executing_task, buffer);
    }

    // Always print to stdout/stderr as well
    va_end(args);
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

void logger_init(void)
{
    // No-op for tests
}

void logger_set_sink_enabled(uint8_t sink_mask, bool enabled)
{
    // No-op for tests
}
