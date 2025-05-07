/**
 * @author  Marc Aaron Reyes
 * @date    2025-05-05
 */

#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include "powerup_task.h"

void powerup_task_init(slate_t* slate){
    LOG_INFO("Powerup task is initializing...");
}

int check_rbf_pin(slate_t* slate){
    return gpio_get(SAMWISE_RBF) == 1;
}

void powerup_task_dispatch(slate_t* slate){
    LOG_INFO("Checking RBF pin...");
    if (!check_rbf_pin(slate)){
        LOG_INFO("RBF pin is low, powerup sequence rejected. Shutting off.");
        return;
    }

    LOG_INFO("RBF pin is pulled...");
}

sched_task_t powerup_task = {.name = "powerup",
                            .dispatch_period_ms = 1000,
                            .task_init = &payload_task_init,
                            .next_dispatch = 0};
