/**
 * @author  Marc Aaron Reyes
 * @date    2025-05-03
 */

#include "payload_task.h"
#define MAX_RECEIVED_LEN 1024

void payload_task_init(slate_t *slate)
{
    LOG_INFO("Payload task is initializing...");

    LOG_INFO("Initializing UART...");
    payload_uart_init(slate);

    LOG_INFO("Turning on Payload...");
    payload_turn_on(slate);

    LOG_INFO("Waiting for Pi to boot up...");
    sleep_ms(10000);
}

void beacon_down_command_test(slate_t *slate)
{
    char packet[] = "[\"send_file_2400\", [\"home/pi/code/main.py\"], {}]";
    int len = sizeof(packet) - 1;
    payload_uart_write_packet(slate, packet, len, 999);

    sleep_ms(1000);

    char received[MAX_RECEIVED_LEN];
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

void ping_command_test(slate_t *slate)
{
    char packet[] = "[\"ping\", [], {}]";
    int len = sizeof(packet) - 1;
    payload_uart_write_packet(slate, packet, len, 999);

    sleep_ms(1000);

    char received[MAX_RECEIVED_LEN];
    uint16_t received_len = payload_uart_read_packet(slate, received);
    if (received_len == 0)
    {
        LOG_INFO("ACK was not received!");
    }
    else
    {
        LOG_INFO("ACK received!");
        LOG_INFO("ACK:");
        for (int i = 0; i < received_len; i++)
        {
            printf("%c", received[i]);
        }
        printf("\n");
    }
}

void payload_task_dispatch(slate_t *slate)
{
    LOG_INFO("Sending an Info Request Command to the RPI...");
    beacon_down_command_test(slate);
    ping_command_test(slate);
}

sched_task_t payload_task = {.name = "payload",
                             .dispatch_period_ms = 1000,
                             .task_init = &payload_task_init,
                             .task_dispatch = &payload_task_dispatch,
                             .next_dispatch = 0};
