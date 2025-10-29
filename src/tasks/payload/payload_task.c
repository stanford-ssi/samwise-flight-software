/**
 * @author  Marc Aaron Reyes
 * @date    2025-05-03
 */

#include "payload_task.h"
#include "neopixel.h"
#include "safe_sleep.h"
#include <stdio.h>

#define MAX_RECEIVED_LEN 1024

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

/* ASSOCIATED PAYLOAD TESTS
 * Types of tests:
 *      - Singular Payload commands
 *      - Bringup
 *      - Functionality
 *      - Error handling
 */

/*** PAYLOAD COMMANDS TESTS ***/
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


void heartbeat_check(slate_t *slate)
{
    bool response_received = ping_command_test(slate);
    bool rpi_enabled = gpio_get_out_level(SAMWISE_RPI_ENAB);
    if(rpi_enabled && response_received){ 
        LOG_INFO("RPi is operational!");
    } else if (rpi_enabled && !response_received){
        // rpi should be on. Serial communication error. 
        LOG_ERROR("Serial Communication is not working! RPI_ENAB Pin is high, but Raspberry Pi is not operational");

    } else if (!rpi_enabled && !response_received){
        if(!slate->is_payload_on){
            // we have called the payload_turn_off() function, so RPI_ENAB is correctly set to false.
            LOG_ERROR("RPi is asleep, as it should be!");
        } else {
            // RPI_ENAB pin was pulled low accidentally (that is, it was pulled low without calling the payload_turn_off() function)
            LOG_ERROR("RPi is asleep, but it should be on... attempting to turn on...");
            payload_turn_on(slate);
        }
    } else {
        // RPi is already on but RPI_ENABLED is off.
        LOG_ERROR("RPi is operational, but RPI_ENAB pin is off. Checking if RPI_ENAB is meant to be turned on...");
        if(!slate->is_payload_on){
            LOG_ERROR("RPI_ENAB is meant to be off!");
        } else {
            LOG_ERROR("RPI_ENAB was not meant to be turned off. Turning it on...");
            payload_turn_on(slate);
            sleep_ms(5);
            if(gpio_get_out_level(SAMWISE_RPI_ENAB)){
                LOG_ERROR("RPI_ENAB pulled to high successfully!");
            } else {
                LOG_ERROR("Unable to pull RPI_ENAB pin to high");
            }
        }
        
    }

}


bool ping_command_test(slate_t *slate)
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
        return false;
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
        return true;
    }
}

/*** BRINGUP TESTS ***/
void power_on_off_payload_test(slate_t *slate)
{
    LOG_INFO("Turning Payload on...");
    payload_turn_on(slate);

    LOG_INFO("Checking to see if slate variable was changed properly...");
    if (slate->is_payload_on)
    {
        LOG_INFO("Slate, is_payload_on, variable was changed properly!");
    }
    else
    {
        LOG_INFO("Slate, is_payload_on, variable was not changed properly, "
                 "ending the test...");
        return;
    }

    LOG_INFO("Sleeping for 10 seconds to let Payload boot up, do not do this "
             "for flight ready version of the software...");
    sleep_ms(10000);

    LOG_INFO("Payload was turned on successfully...");
    LOG_INFO("Testing Payload turning off...");

    LOG_INFO("Turning off Payload...");
    payload_turn_off(slate);

    LOG_INFO("Checking to see if slate variable was changed properly...");
    if (!slate->is_payload_on)
    {
        LOG_INFO("Slate, is_payload_on, variable was changed properly!");
    }
    else
    {
        LOG_INFO("Slate, is_payload_on, variable was not changed properly, "
                 "ending the test...");
        return;
    }

    LOG_INFO("Checking RPI_ENAB pin to see if it reads 0...");
    if (!gpio_get_out_level(SAMWISE_RPI_ENAB))
    {
        LOG_INFO("RPI_ENAB is pulled low...");
    }

    LOG_INFO("Test ran successfully, exiting test...");
}

/** Functionality Tests **/

void payload_uart_write_off_test(slate_t *slate)
{
    // NOTE: Mostly visual, run without RPi harness connected onto RPi
    char packet[] = "[\"ping\", [], {}]";
    int len = sizeof(packet) - 1;

    payload_write_error_code res =
        payload_uart_write_packet(slate, packet, len, 999);

    LOG_INFO("This should print, means it exited properly...");

    if (res != SUCCESSFUL_WRITE)
    {
        LOG_INFO("Sucessful exit code...");
    }
    else
    {
        LOG_INFO("Something is very wrong...");
    }
}

void payload_uart_write_on_test(slate_t *slate)
{
    char packet[] = "[\"ping\", [], {}]";
    int len = sizeof(packet) - 1;

    payload_write_error_code res =
        payload_uart_write_packet(slate, packet, len, 999);

    LOG_INFO("This should print, means it exited properly...");

    if (res == SUCCESSFUL_WRITE)
    {
        LOG_INFO("Sucessful exit code...");
    }
    else
    {
        LOG_INFO("Something is very wrong...");
    }
}

void payload_task_dispatch(slate_t *slate)
{
    neopixel_set_color_rgb(PAYLOAD_TASK_COLOR);
    LOG_INFO("Sending an Info Request Command to the RPI...");
    // beacon_down_command_test(slate);
    // ping_command_test(slate);

    payload_uart_write_on_test(slate);

    return;

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
