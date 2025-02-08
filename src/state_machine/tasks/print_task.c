
/**
 * @author  Niklas Vainio
 * @date    2024-08-25
 */

#include "print_task.h"
#include "drivers/payload_uart.h"
#include "macros.h"
#include "slate.h"

void print_task_init(slate_t *slate)
{
    LOG_INFO("Test task is initializing...");
}

void print_task_dispatch(slate_t *slate)
{
    LOG_INFO("Sending a command to the RPI...");

    char packet[] = "['ping', [], {}]";
    int len = sizeof(packet) - 1;
    payload_uart_write_packet(slate, packet, len, 999);

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

sched_task_t print_task = {.name = "print",
                           .dispatch_period_ms = 1000,
                           .task_init = &print_task_init,
                           .task_dispatch = &print_task_dispatch,

                           /* Set to an actual value on init */
                           .next_dispatch = 0};
