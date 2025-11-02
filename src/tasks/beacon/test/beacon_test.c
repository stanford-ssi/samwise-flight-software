/**
 * @author  Yao Yiheng
 * @date    2025-01-25
 *
 * Test for beacon task.
 */

#include "adcs_packet.h"
#include "beacon_task.h"
#include <stdio.h>

/**
 * Statically allocate the slate.
 */
slate_t slate;
uint8_t tmp_data[252];

sched_state_t mock_state = {
    .name = "mock_state",
    .num_tasks = 0,
    .task_list = {NULL},
    .get_next_state = NULL,
};

void mock_slate(slate_t *slate)
{
    // Reset slate to empty first
    memset(slate, 0, sizeof(slate_t));

    // Mock values into slate
    slate->time_in_current_state_ms = 12345;
    slate->current_state = &mock_state;
    slate->adcs_telemetry = (adcs_packet_t){
        .w = 1.0,
        .q0 = 0.1,
        .q1 = 0.2,
        .q2 = 0.3,
        .q3 = 0.4,
        .state = 'A',
        .boot_count = 42,
    };
}

void test_beacon_serialize()
{
    printf("Starting beacon serialization test\n");
    mock_slate(&slate);
    size_t len = serialize_slate(&slate, tmp_data);
    printf("Serialized length: %zu\n", len);
    printf("Serialized data (hex): ");
    for (size_t i = 0; i < len; i++)
    {
        printf("%02x ", tmp_data[i]);
    }
    ASSERT(strcmp((char *)tmp_data, "mock_state") == 0);
    printf("\n");
}

void test_beacon_dispatch_without_error()
{
    printf("Starting beacon dispatch test\n");
    mock_slate(&slate);
    beacon_task_init(&slate);
    beacon_task_dispatch(&slate);
    printf("Beacon dispatch completed without error\n");
}

int main()
{
    test_beacon_serialize();
    test_beacon_dispatch_without_error();
    return 0;
}
