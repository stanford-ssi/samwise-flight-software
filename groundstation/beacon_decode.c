typedef struct
{
    uint32_t reboot_counter;
    uint64_t time;
    uint32_t rx_bytes;
    uint32_t rx_packets;
    uint32_t rx_backpressure_drops;
    uint32_t rx_bad_packet_drops;
    uint32_t tx_bytes;
    uint32_t tx_packets;
} __attribute__((__packed__)) beacon_stats;

int decode_beacon(const uint8_t *data)
{
    const char *name = (const char *)data;
    size_t name_len = strlen(name);

    beacon_stats stats;
    memcpy(&stats, data + name_len, sizeof(stats));

    return 0;
}