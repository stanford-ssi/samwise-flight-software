/**
 * @author  Yao Yiheng
 * @date    2025-01-25
 *
 * Test for beacon task.
 */

#include "adcs_packet.h"
#include "beacon_task.h"
#include "error.h"
#include "logger.h"
#include "state_registry.h"
#include <stdio.h>
#include <stdlib.h>

/**
 * Statically allocate the slate.
 */
slate_t slate;
uint8_t tmp_data[252];

static state_id_t mock_get_next_state(slate_t *s)
{
    return STATE_INIT;
}

sched_state_t mock_state = {
    .name = "mock_state",
    .id = STATE_INIT,
    .num_tasks = 0,
    .task_list = {NULL},
    .get_next_state = mock_get_next_state,
};

void mock_slate(slate_t *slate)
{
    // Reset slate to empty first
    clear_and_init_slate(slate);

    // Register mock state so state_registry_get can find it
    state_registry_register(STATE_INIT, &mock_state);

    // Mock values into slate
    slate->time_in_current_state_ms = 12345;
    slate->current_state_id = STATE_INIT;
    slate->adcs_telemetry = (adcs_packet_t){
        .w = 1.0,
        .q0 = 0.1,
        .q1 = 0.2,
        .q2 = 0.3,
        .q3 = 0.4,
        .state = 'A',
        .boot_count = 42,
    };
    slate->reboot_counter = 42;
    slate->battery_voltage = 4000;
}

void test_beacon_serialize()
{
    printf("Starting beacon serialization test\n");
    mock_slate(&slate);

    size_t len = serialize_slate(&slate, tmp_data);
    printf("Serialized length: %zu\n", len);
    printf("Serialized data (hex):\n");
    for (size_t i = 0; i < len; i++)
    {
        printf("%02x ", tmp_data[i]);
        if (i % 10 == 9)
            printf("\n");
    }
    ASSERT(strcmp((char *)tmp_data, "mock_state beat cal!") == 0);
    printf("\n");

    // Write hex artifact to TEST_UNDECLARED_OUTPUTS_DIR if set by Bazel
    const char *outputs_dir = getenv("TEST_UNDECLARED_OUTPUTS_DIR");
    if (outputs_dir)
    {
        char path[512];
        snprintf(path, sizeof(path), "%s/beacon_packet.hex", outputs_dir);
        FILE *f = fopen(path, "w");
        if (f)
        {
            for (size_t i = 0; i < len; i++)
            {
                if (i % 10 == 9 || i == len - 1)
                {
                    fprintf(f, "%02x\n", tmp_data[i]);
                }
                else
                {
                    fprintf(f, "%02x ", tmp_data[i]);
                }
            }
            fclose(f);
        }
    }

    free_slate(&slate);
}

void test_beacon_dispatch_without_error()
{
    printf("Starting beacon dispatch test\n");
    mock_slate(&slate);

    beacon_task_init(&slate);
    beacon_task_dispatch(&slate);
    printf("Beacon dispatch completed without error\n");

    free_slate(&slate);
}

int main()
{
    test_beacon_serialize();
    test_beacon_dispatch_without_error();
    return 0;
}
