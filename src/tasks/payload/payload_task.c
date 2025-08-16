/**
 * @author  Marc Aaron Reyes
 * @date    2025-05-03
 */

#include "payload_task.h"
#include "payload_tests.h"
#include "safe_sleep.h"

#define MAX_RECEIVED_LEN 1024

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
            payload_write_error_code exec_successful =
                payload_uart_write_packet(
                    slate, payload_command.serialized_command,
                    sizeof(payload_command.serialized_command),
                    payload_command.seq_num);
            // If the command was successful, remove it from the queue.
            // Alternatively, if we have already retried the command up to
            // a maximum number of times, remove it from the queue.
            if (exec_successful == SUCCESSFUL_WRITE ||
                RETRY_COUNT >= MAX_PAYLOAD_RETRY_COUNT)
            {
                // Return success when the command is removed.
                return queue_try_remove(&slate->payload_command_data,
                                        &payload_command);
            }
            else if (exec_successful == PACKET_TOO_BIG)
            {
                LOG_DEBUG("Packet exceeds 4096 bytes...");
                return false;
            }
            else if (exec_successful == SYN_UNSUCCESSFUL)
            {
                LOG_DEBUG("PiCubed was unable to sync with the Payload...");
                return false;
            }
            else if (exec_successful == UART_WRITE_TIMEDOUT)
            {
                LOG_DEBUG("The transmission took too long and the write timed "
                          "out...");
                return false;
            }
            else if (exec_successful == HEADER_UNACKNOWLEDGED)
            {
                LOG_DEBUG("Payload did not acknowledge the header...");
                return false;
            }
            else if (exec_successful == FINAL_WRITE_UNSUCCESSFUL)
            {
                LOG_DEBUG("Final packet transmission timed out...");
                return false;
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

#ifdef PAYLOAD_ONLY
void payload_task_init(slate_t *slate)
{
    LOG_INFO("Payload task is initializing...");

    LOG_INFO("Initializing UART...");
    if (slate->is_uart_init)
    {
        LOG_INFO("UART already initialized...");
    }
    else
    {
        payload_uart_init(slate);
        slate->is_uart_init = true;
        LOG_INFO("UART has been initialized, please turn on Payload separately "
                 "before doing any payload commands...");
    }

    // Forcing Payload to turn on (Only do this for testing)
    payload_turn_on(slate);

    LOG_INFO("Forcing Payload to turn on, sleeping for 10 Seconds");
    sleep_ms(10000);
}

void payload_task_dispatch(slate_t *slate)
{
    LOG_INFO("Sending an Info Request Command to the RPI...");

    if (!slate->is_payload_on)
    {
        LOG_ERROR("Payload is turned off, please force payload to turn on...");
    }

    if (slate->command_override && slate->is_payload_on)
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
    else if (slate->is_payload_on)
    {
        LOG_INFO("Running proctored unit tests...");
    }
    else
    {
        LOG_INFO("Payload is OFF, not executing commands.");
    }
}

#else

void payload_task_init(slate_t *slate)
{
    LOG_INFO("Payload task is initializing...");

    LOG_INFO("Initializing UART...");
    if (slate->is_uart_init)
    {
        LOG_INFO("UART already initialized...");
    }
    else
    {
        payload_uart_init(slate);
        slate->is_uart_init = true;
        LOG_INFO("UART has been initialized, please turn on Payload separately "
                 "before doing any payload commands...");
    }
    // NOTE: Turning on payload is handled by command_parser

    // Forcing Payload to turn on (Only do this for testing)
    payload_turn_on(slate);

    LOG_INFO("Forcing Payload to turn on, sleeping for 10 Seconds");
    sleep_ms(10000);
}

void payload_task_dispatch(slate_t *slate)
{
    LOG_INFO("Sending an Info Request Command to the RPI...");

    char packet[] = "[\"ping\", [], {}]";
    payload_uart_write_packet(slate, packet, sizeof(packet) - 1, 999);

    sleep_ms(100);

    char received_packet[1024];
    uint16_t received_len = payload_uart_read_packet(slate, received_packet);

    if (received_len != 0)
    {
        LOG_INFO("ACK received!");
        LOG_INFO("ACK out: %s", received_packet);
    }

    return;

    if (!slate->is_payload_on)
    {
        LOG_ERROR("Turn the payload on first before starting the test.");
    }

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
}

#endif

sched_task_t payload_task = {.name = "payload",
                             .dispatch_period_ms = 1000,
                             .task_init = &payload_task_init,
                             .task_dispatch = &payload_task_dispatch,
                             .next_dispatch = 0};
