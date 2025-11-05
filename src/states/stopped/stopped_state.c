#include "stopped_state.h"
#include "device_status.h"
#include "payload.h" 
#include "logging.h"

// Add safety mech, like only if called 3 times
#define NUM_STOP_CONFIRMATIONS 3

static int stop_command_count = 0;
static bool stop_command_received(slate_t *slate) {
    (void)slate; // unused parameter

    #ifdef DEVICE_STATUS_STOP_REQUESTED 
        return is_device_status_flag_set(DEVICE_STATUS_STOP_REQUESTED);
    #else
        return false;
    #endif
}
sched_state_t *stop_get_next_state(slate_t *slate) {
    if (!stop_command_received(slate)) {
        stop_command_count = 0; // reset count if no stop command
        return NULL;

    } else {
        stop_command_count++;
        if (stop_command_count >= NUM_STOP_CONFIRMATIONS) {
            stop_command_count = 0; // reset after confirming stop
            return &stop_state; // remain in stopped state
        }
    }
    return NULL; // stay in current state until conditions change
}

void stop_state_enter(slate_t *slate) {
    LOG_WARN("Entering STOP state. Disabling mission operations.");
    
    payload_turn_off(slate); 
    }

void stop_state_exit(slate_t *slate)
    {
    LOG_INFO("Exiting STOP state.");
    }

void stop_state_update(slate_t *slate)
{
    // Keep satellite in a minimal-power safe idle.
    // Only listen for further commands.
}
                                  
sched_state_t stop_state = {
    .name = "stop_state",
    .enter_state = stop_state_enter,
    .exit_state = stop_state_exit,
    .update_state = stop_state_update,
};
