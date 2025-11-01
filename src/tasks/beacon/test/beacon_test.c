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
        .state = 'D',
        .boot_count = 42,
        .q_eci_to_body_x = 1.0,
        .q_eci_to_body_y = 0.0,
        .q_eci_to_body_z = 0.0,
        .q_eci_to_body_w = 0.0,
        .w_body_x = 0.0,
        .w_body_y = 0.0,
        .w_body_z = 0.0,
        .adcs_power = 42.0,
        .sun_vector_body_x = 1.0,
        .sun_vector_body_y = 0.0,
        .sun_vector_body_z = 0.0,
        .b_body_raw_x = 0.0,
        .b_body_raw_y = 0.0,
        .b_body_raw_z = 0.0,
        .magnetorquer_moment_x = 0.0,
        .magnetorquer_moment_y = 0.0,
        .magnetorquer_moment_z = 0.0,
        .w_reaction_wheels_0 = 0.0,
        .w_reaction_wheels_1 = 0.0,
        .w_reaction_wheels_2 = 0.0,
        .w_reaction_wheels_3 = 0.0,
        .magnetometer_data_valid = true,
        .gps_data_valid = true,
        .imu_data_valid = true,
        .sun_vector_valid = true,
        .sun_sensor_data_valid = {true, true, true, true, true, true, true,
                                  true, true, true, true, true, true, true,
                                  true, true},
        .P_log_frobenius = 0.1f,
        .lat = 37.7749f,
        .lon = -122.4194f,
        .alt = 500.0f,
        .mjd = 59580.0f,
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
