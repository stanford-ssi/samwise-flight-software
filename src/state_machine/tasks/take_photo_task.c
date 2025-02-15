#include "take_photo_task.h"
#include "drivers/payload_uart.h"
#include "macros.h"
#include "slate.h"

void take_photo_task_init(slate_t *slate){
    LOG_INFO("Test task is initializing...");
}

void take_photo_task_dispatch(slate_t *slate){
    
    LOG_INFO("Sending a command to the RPI...");

    char packet[] = "";
    int len = sizeof(packet) - 1;
    payload_uart_write_packet(slate, packet, len, 999);

    char received[1024];

}

sched_task_t take_photo_task = {.name = "photo",
                                .dispatch_period_ms = 1000,
                                .task_init = &take_photo_task_init,
                                .task_dispatch = &take_photo_task_dispatch,

                                /* Set to an actual value on init*/
                                .next_dispatch = 0};
