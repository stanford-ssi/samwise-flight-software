// TODO
#include "ftp_task.h"
#include "neopixel.h"

void ftp_task_init(slate_t *slate)
{
    LOG_INFO("FTP task is initializing...");
    // Add initialization code here
}

void ftp_task_dispatch(slate_t *slate)
{
    neopixel_set_color_rgb(FTP_TASK_COLOR);
    
    // Add your task logic here
    LOG_INFO("FTP task is running...");
    
    neopixel_set_color_rgb(0, 0, 0);
}

sched_task_t ftp_task = {
    .name = "ftp_task",
    .dispatch_period_ms = 1000,  // TODO: Adjust period as needed
    .task_init = &ftp_task_init,
    .task_dispatch = &ftp_task_dispatch,
    .next_dispatch = 0
};