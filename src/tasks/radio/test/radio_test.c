#include "radio_task.h"
#include <stdio.h>

/**
 * Test for radio task.
 */

void test_encode_packet_basic()
{
    printf("Starting basic encode_packet test\n");
    // Arrange.
    packet_t p = {.dst = 1,
                  .src = 2,
                  .flags = 0,
                  .seq = 1,
                  .len = 3,
                  .data = {0xAA, 0xBB, 0xCC},
                  .boot_count = 42,
                  .msg_id = 7,
                  .hmac = {0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x11, 0x22, 0x33}};
    uint8_t buf[PACKET_SIZE] = {0};

    printf("Data Length: %u\n", sizeof(p.data));

    // Act.
    size_t encoded_size = encode_packet(&p, buf, sizeof(buf), true);
    if (encoded_size == 0)
    {
        printf("encode_packet failed\n");
        return;
    }
    printf("Encoded size: %zu\n", encoded_size);
    printf("Encoded data (hex): ");
    for (size_t i = 0; i < encoded_size; i++)
    {
        printf("%02x ", buf[i]);
    }

    // Assert.
    ASSERT(buf[4] == 3);    // Length byte
    printf("%u\n", buf[8]); // Data byte 1
    ASSERT(buf[8] == 42);   // Boot count LSB
    printf("\n");
}

int main()
{
    printf("Starting radio test\n");
    test_encode_packet_basic();
    return 0;
}
