/**
 * @author Marc Aaron Reyes
 * @date 2025-05-21
 *
 * An easier way to send commands via UART to the Payload
 */

#include "payload_command.h"
#include "payload_uart.h"

#define MAX_RECEIVED_LEN 1024
#define COMMAND_NUM 1

static int write_count = 0;

char *ping_cmd()
{
    char *command = "[\"ping\", [], {}]";
    return command;
}

static const command_t commands[] = {
    {"ping", ping_cmd},
};

bool init_payload(slate_t *slate)
{
    LOG_INFO("Initializing UART protocols with Payload...");
    payload_uart_init(slate);

    LOG_INFO("Turning on RPi Payload...");
    payload_turn_on(slate);

    LOG_INFO("Waiting for RPi to turn on....");
    sleep_ms(10000);

    LOG_INFO("Payload is turned on...");

    return true;
}

uint16_t create_seq_num()
{
    return write_count;
}

int strcmp(const char *s1, const char *s2)
{
    // Loops through each index of both char*
    int ptr = 0;
    while ((s1[ptr] != '\0') || (s2[ptr] != '\0'))
    {

        // Finds the difference between their ASCII values
        if (s1[ptr] != s2[ptr])
        {
            return s1[ptr] - s2[ptr];
        }

        // Moves to the next index
        ptr++;
    }

    // Only ever returns 0 when the strings are the same
    return 0;
}

int verify_command(const uint8_t *command)
{
    int command_index = 0;
    int command_found_index = -1;
    while (command_index < COMMAND_NUM)
    {
        if (strcmp(commands[command_index].name, command) == 0)
        {
            command_found_index = command_index;
            break;
        }

        command_index++;
    }

    return 1;
}

char *format_command(const uint8_t *command)
{
    int cmd_index = verify_command(command);
    if (cmd_index == -1)
    {
        return NULL;
    }

    return commands[cmd_index].fn();
}

uint16_t payload_send_command(slate_t *slate, const uint8_t *command,
                              uint8_t *report)
{
    char *command_packet = format_command(command);

    if (command_packet == NULL)
    {
        report = "Invalid command!";
        LOG_INFO("Command not valid!");
        return 0;
    }

    int packet_len = sizeof(command_packet) - 1;
    uint16_t seq_num = create_seq_num();

    write_count++;

    payload_uart_write_packet(slate, command_packet, packet_len, seq_num);

    sleep_ms(1000);

    char received_msg[MAX_RECEIVED_LEN];
    uint16_t received_msg_len = payload_uart_read_packet(slate, received_msg);
    if (received_msg_len == 0)
    {
        LOG_INFO("An ACK was not received from Payload, Payload may not be "
                 "responding...");
    }
    else
    {
        LOG_INFO("ACK received!");
        return received_msg_len;
    }

    return 0;
}
