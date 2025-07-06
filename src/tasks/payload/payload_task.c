/**
 * @author  Marc Aaron Reyes
 * @date    2025-05-03
 */

#include "payload_task.h"
#include "neopixel.h"
#include "safe_sleep.h"

#define MAX_RECEIVED_LEN 1024

void payload_task_init(slate_t *slate)
{
    LOG_INFO("Payload task is initializing...");

    LOG_INFO("Initializing UART...");
    payload_uart_init(slate);

    LOG_INFO("Turning on Payload...");
    payload_turn_on(slate);

    // TODO: initialization needs to be moved into a separate function
    // init is only executed once, but we should toggle RPi on only when
    // commands need to be executed.
    LOG_INFO("Waiting for Pi to boot up...");
    safe_sleep_ms(10000);
}

void beacon_down_command_test(slate_t *slate)
{
    char packet[] = "[\"send_file_2400\", [\"home/pi/code/main.py\"], {}]";
    int len = sizeof(packet) - 1;
    payload_uart_write_packet(slate, packet, len, 999);

    safe_sleep_ms(1000);

    char received[MAX_RECEIVED_LEN];
    uint16_t received_len = payload_uart_read_packet(slate, received);

    if (received_len == 0)
    {
        LOG_INFO("Did not received anything!");
    }
    else
    {
        LOG_INFO("Received something:");
        for (uint16_t i = 0; i < received_len; i++)
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

    safe_sleep_ms(1000);

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
        for (uint16_t i = 0; i < received_len; i++)
        {
            printf("%c", received[i]);
        }
        printf("\n");
    }
}

bool try_execute_payload_command(slate_t *slate)
{
    if (!queue_is_empty(&slate->payload_command_data))
    {
        PAYLOAD_COMMAND_DATA payload_command;
        if (queue_try_peek(&slate->payload_command_data, &payload_command))
        {
            LOG_INFO("Executing Payload Command: %s",
                     payload_command.serialized_command);
            // First attempt to execute the command but do not throw it away
            // yet.
            bool exec_successful = payload_uart_write_packet(
                slate, payload_command.serialized_command,
                sizeof(payload_command.serialized_command),
                payload_command.seq_num);
            // If the command was successful, remove it from the queue.
            // Alternatively, if we have already retried the command up to
            // a maximum number of times, remove it from the queue.
            if (exec_successful || RETRY_COUNT >= MAX_PAYLOAD_RETRY_COUNT)
            {
                // Return success when the command is removed.
                return queue_try_remove(&slate->payload_command_data,
                                        &payload_command);
            }
            else
            {
                LOG_DEBUG("Payload command execution failed, retrying...");
                // If the command was not successful, we will not remove it
                // from the queue and will try again next time.
                return false;
            }
        }
        else
        {
            LOG_DEBUG("Failed to peek payload command from queue.");
            return false;
        }
    }
    // No payload commands to execute
    return true;
}

void payload_task_dispatch(slate_t *slate)
{
    neopixel_set_color_rgb(PAYLOAD_TASK_COLOR);
    LOG_INFO("Sending an Info Request Command to the RPI...");
    beacon_down_command_test(slate);
    ping_command_test(slate);

    if (slate->is_payload_on)
    {
        LOG_INFO("Payload is ON, executing commands...");
        // Attempts to execute k commands per dispatch.
        for (int k = 0; k < MAX_PAYLOAD_COMMANDS_PER_DISPATCH; k++)
        {
            // Execute pending payload commands.
            if (!try_execute_payload_command(slate))
            {
                RETRY_COUNT++;
                break;
            }
            else
            {
                // Reset retry count if command was successfully executed
                RETRY_COUNT = 0;
            }
        }
    }
    else
    {
        LOG_INFO("Payload is OFF, not executing commands.");
    }
    neopixel_set_color_rgb(0, 0, 0);
}

sched_task_t payload_task = {.name = "payload",
                             .dispatch_period_ms = 1000,
                             .task_init = &payload_task_init,
                             .task_dispatch = &payload_task_dispatch,
                             .next_dispatch = 0};
