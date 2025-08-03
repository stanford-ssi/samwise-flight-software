#include payload_tests.h

#define MAX_BUF_LEN 1024

void run_test_sequence()
{
}

bool run_test(slate_t *slate, char *packet, int packet_len, bool verbose)
{
    payload_uart_write_packet(slate, packet, len, 999);

    safe_sleep_ms(1000);

    char received_packet[MAX_BUF_LEN];
    uint16_t received_len = payload_uart_read_packet(slate, received);

    if (!verbose)
    {
        return received_len;
    }

    if (received_len == 0)
    {
        LOG_ERROR("ACK was not received!");
        return false;
    }

    LOG_INFO("ACK received!");
    LOG_INFO("ACK out: %s", received);
    return true;
}

bool ping_command_test(slate_t *slate)
{
    char packet[] = "[\"ping\", [], {}]";
    return run_test(slate, packet, sizeof(packet) - 1);
}

bool take_photo_command_test(slate_t *slate, char *file_name, char *args,
                             bool verbose)
{
    char packet_buf[MAX_BUF_LEN];
    snprintf(packet_buf, MAX_BUF_LEN, "[\"take_photo\", [\"%s\"], {%s}",
             file_path, args);

    return run_test(slate, packet_buf, sizeof(packet_buf) - 1);
}

bool send_2400_command_test(slate_t *slate, char *file_path, char *args,
                            bool verbose)
{
    char packet_buf[MAX_BUF_LEN];
    snprintf(packet_buf, MAX_BUF_LEN, "[\"send_file_2400\", [\"%s\"], {}",
             file_path, args);

    return run_test(slate, packet, sizeof(packet) - 1);
}

bool power_on_off_payload_test(slate_t *slate)
{
    LOG_INFO("Turning Payload on...");
    payload_turn_on(slate);

    LOG_INFO("Checking to see if salte variable was changed properly...");
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

bool payload_camera_breadth_test(slate_t *slate, char *file_name,
                                 char *photo_args, char *downlink_args)
{
    LOG_INFO("Executing test: payload_camera_breadth_test...");
    LOG_INFO("Task List: take_photo, send_2400_file\n");

    LOG_INFO("Current Task: take_photo");
    return true;
}
