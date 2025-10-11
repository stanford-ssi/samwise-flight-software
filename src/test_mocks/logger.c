#include "logger.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// Track currently executing task for log capture
const char *current_executing_task = NULL;

// External reference to viz log (from test_scheduler_helpers.c)
extern FILE *viz_log;
extern uint64_t mock_time_us;

static void log_viz_task_message(const char *task_name, const char *log_message)
{
    if (viz_log != NULL && task_name != NULL && log_message != NULL)
    {
        uint32_t time_ms = (uint32_t)(mock_time_us / 1000);

        // Escape quotes in log message
        char escaped[512];
        size_t j = 0;
        for (size_t i = 0; log_message[i] != '\0' && j < sizeof(escaped) - 2; i++)
        {
            if (log_message[i] == '"')
            {
                escaped[j++] = '\\';
            }
            else if (log_message[i] == '\n')
            {
                continue; // Skip newlines
            }
            escaped[j++] = log_message[i];
        }
        escaped[j] = '\0';

        fprintf(viz_log,
                "{\"time_ms\": %u, \"event\": \"task_log\", \"task\": \"%s\", "
                "\"details\": \"%s\"},\n",
                time_ms, task_name, escaped);
        fflush(viz_log);
    }
}

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
