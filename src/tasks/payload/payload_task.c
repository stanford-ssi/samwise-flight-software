/**
 * @author  Marc Aaron Reyes
 * @date    2025-05-03\
 */

#include "payload_task.h"

void payload_task_init(slate_t *slate){
    LOG_INFO("Payload task is initializing...");
    payload_uart_init(slate);
    payload_turn_on(slate);
}

void payload_task_dispatch(slate_t *slate){
    sleep_ms(10000);
    LOG_INFO("Sending an Info Request Command to the RPI...");
    char packet[] = "[\"send_file_2400\", [\"home/pi/code/main.py\"], {}]";
    int len = sizeof(packet) - 1;
    payload_uart_write_packet(slate, packet, len, 999);

    sleep_ms(1000);

    char received[1024];
    uint16_t received_len = payload_uart_read_packet(slate, received);

    if (received_len == 0)
    {
        LOG_INFO("Did not received anything!");
    }
    else
    {
        LOG_INFO("Received something:");
        for (int i = 0; i < received_len; i++)
        {
            printf("%c", received[i]);
        }
        printf("\n");
    }
}

sched_task_t payload_task = {.name = "payload",
                            .dispatch_period_ms = 1000,
                            .task_init = &payload_task_init,
                            .task_dispatch = &payload_task_dispatch,
                            .next_dispatch = 0};
