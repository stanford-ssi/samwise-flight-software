#include "sd-card_task.h"

FRESULT fr;
FATFS fs;
FIL fil;
char filename[] = "test02.txt";

void sdcard_task_init() {}

void sdcard_task_test_rw(){}

sched_task_t sdcard_task = {.name = "sd-card",
                           .dispatch_period_ms = 3000,
                           .task_init = &sdcard_task_init,
                           .task_dispatch = &sdcard_task_test_rw,

                           /* Set to an actual value on init */
                           .next_dispatch = 0};
