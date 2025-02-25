/**
 * @author  Yao Yiheng
 * @date    2025-01-18
 *
 * Task to emit telemetry packet to the radio TX queue.
 */

#include "beacon_task.h"

// Statically allocated local variables
size_t pkt_len;
uint8_t tmp_data[252];

void beacon_task_init(slate_t *slate)
{
    pkt_len = 0;
    memset(tmp_data, 0, sizeof(tmp_data));
}

void beacon_task_dispatch(slate_t *slate)
{
    // Create a new packet for radio TX
    packet_t pkt;
    pkt.src = 0;
    pkt.dst = 0;
    pkt.flags = 0;
    pkt.seq = 0;

    // Serialize data from slate variables
    // Current state name
    size_t name_len = strlen(slate->current_state->name);
    for (pkt_len = 0; pkt_len < name_len; ++pkt_len)
    {
        tmp_data[pkt_len] = slate->current_state->name[pkt_len];
    }
    tmp_data[pkt_len++] = ',';

    // Current time
    tmp_data[pkt_len++] = slate->time_in_current_state_ms & 0x11111111;
    tmp_data[pkt_len++] = (slate->time_in_current_state_ms >> 8) & 0x11111111;
    tmp_data[pkt_len++] = (slate->time_in_current_state_ms >> 16) & 0x11111111;
    tmp_data[pkt_len++] = (slate->time_in_current_state_ms >> 24) & 0x11111111;
    tmp_data[pkt_len++] = (slate->time_in_current_state_ms >> 32) & 0x11111111;
    tmp_data[pkt_len++] = (slate->time_in_current_state_ms >> 40) & 0x11111111;
    tmp_data[pkt_len++] = (slate->time_in_current_state_ms >> 48) & 0x11111111;
    tmp_data[pkt_len++] = (slate->time_in_current_state_ms >> 56) & 0x11111111;

    // Commit into serialized byte array
    pkt.len = pkt_len;
    memcpy(pkt.data, tmp_data, pkt.len);

    // Write into tx_queue
    if (queue_try_add(&slate->tx_queue, &pkt))
    {
        LOG_INFO("Beacon pkt added to queue");
    }
    else
    {
        LOG_ERROR("Beacon pkt failed to commit to tx_queue");
    }
}

sched_task_t beacon_task = {.name = "beacon",
                            .dispatch_period_ms = 30000,
                            .task_init = &beacon_task_init,
                            .task_dispatch = &beacon_task_dispatch,

                            /* Set to an actual value on init */
                            .next_dispatch = 0};
