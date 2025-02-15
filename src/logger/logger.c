#include "logger.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>

// Track enabled sinks using bitwise OR
static uint8_t enabled_sinks = LOG_SINK_TEST | LOG_SINK_FLASH | LOG_SINK_DISK;

// Initialize logger system
void logger_init(void) {
    // Initialize hardware/drivers for FLASH/DISK if needed
    #ifndef TEST
        // TODO: Initialize flash logging when ready
        // TODO: Initialize disk logging when ready
    #endif
}

// Enable/disable specific sinks
void logger_set_sink_enabled(uint8_t sink_mask, bool enabled) {
    if (enabled) {
        enabled_sinks |= sink_mask;    // Set bits using OR
    } else {
        enabled_sinks &= ~sink_mask;   // Clear bits using AND with inverted mask
    }
}

// Main logging function
void log_message(LOG_LEVEL level, uint8_t sink_mask, const char* fmt, ...) {
    // Only log to enabled sinks
    sink_mask &= enabled_sinks;
    
    if (!sink_mask) {
        return;
    }

    // Format the message
    char buffer[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    // Output to each enabled sink
    if (sink_mask & LOG_SINK_TEST) {
        printf("%s", buffer);
    }

    if (sink_mask & LOG_SINK_FLASH) {
        // TODO: Implement flash logging
    }

    if (sink_mask & LOG_SINK_DISK) {
        // TODO: Implement disk logging
    }
}