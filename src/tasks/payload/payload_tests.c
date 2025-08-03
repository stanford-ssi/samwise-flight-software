#include payload_tests.h

#define MAX_BUF_LEN 1024

void run_test_sequence(size_t n, const payload_unit_test_t *fns, char *seq_name)
{
    LOG_INFO("Executing test: %s...", seq_name);
    LOG_INFO("Tasks to Execute: %d\n", n);

    int tasks_successful = 0;
    for (int i = 0; i < n; i++)
    {
        payload_unit_test_t *curr_unit = &fns[i];
        LOG_INFO("Executing task ID [%d]: %s", i, curr_unit->fn_name);
        bool ret = curr_unit->fn(curr_unit->p_args, curr_unit->s_args,
                                 curr_unit->flags);

        if (!ret)
        {
            LOG_INFO("Task ID [%d] failed...");
        }

        tasks_successful++;
        LOG_INFO("Task ID [%d] successful...\n");
    }

    LOG_INFO("Test overview: ");
    LOG_INFO("Tasks successful: %d/%d", tasks_successful, n);
    LOG_INFO("Test concluding...");
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

bool ping_command_test(slate_t *slate, char *p, char *s, char *f)
{
    (void)p;
    (void)s;
    char packet[] = "[\"ping\", [], {}]";
    return run_test(slate, packet, (sizeof(packet) - 1), f != NULL);
}

bool take_photo_command_test(slate_t *slate, char *p, char *s, char *f)
{
    char packet_buf[MAX_BUF_LEN];
    snprintf(packet_buf, MAX_BUF_LEN, "[\"take_photo\", [%s], {%s}", p, s);

    return run_test(slate, packet_buf, (sizeof(packet_buf) - 1), f != NULL);
}

bool send_2400_command_test(slate_t *slate, char *p, char *s, char *f);
{
    (void)s;
    char packet_buf[MAX_BUF_LEN];
    snprintf(packet_buf, MAX_BUF_LEN, "[\"send_file_2400\", [%s], {}", p);

    return run_test(slate, packet, (sizeof(packet) - 1), f != NULL);
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
    return true;
}
