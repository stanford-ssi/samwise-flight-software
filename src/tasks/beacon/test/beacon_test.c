/**
 * @author  Yao Yiheng
 * @date    2025-01-25
 *
 * Test for beacon task.
 */

#include "beacon_test.h"

/**
 * Statically allocate the slate.
 */
slate_t slate;
uint8_t tmp_data[252];

void mock_slate(slate_t *slate)
{
    slate->time_in_current_state_ms = 12345;
}

int main()
{
    printf("Starting beacon test");
    mock_slate(&slate);
    beacon_task_init(&slate);
    printf("Beacon Initialized.\nMocked current time: %d\n",
           slate.time_in_current_state_ms);
    beacon_task_dispatch(&slate);
    serialize_slate(&slate, tmp_data);
    uint64_t time;
    read_uint64(tmp_data, 0, time);
    printf("Serialized time: %d\n", time);
    return 0;
}
