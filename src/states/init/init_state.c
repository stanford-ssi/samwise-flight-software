#include "init_state.h"
#include "device_status.h"
#include "logger.h"
#include "neopixel.h"
#include "states.h"

sched_state_t *init_get_next_state(slate_t *slate)
{
#ifdef BRINGUP
    return &bringup_state;
#else
    // Check if RBF pin is detected
    if (is_rbf_pin_detected())
    {
        // If RBF pin is detected, block and stay in init state
        LOG_INFO("RBF pin detected, staying in init state.");
        neopixel_set_color_rgb(0xff, 0, 0);
        return &init_state;
    }
    return &running_state;
#endif
}

sched_state_t init_state = {.name = "init",
                            .num_tasks = 0,
                            .task_list = {},
                            .get_next_state = &init_get_next_state};
