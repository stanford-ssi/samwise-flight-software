#include <stdio.h>
#include <stdarg.h>
#include "logger.h"

void log_message(LOG_LEVEL level, uint8_t sink_mask, const char *fmt, ...)
{
    va_list args;
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
