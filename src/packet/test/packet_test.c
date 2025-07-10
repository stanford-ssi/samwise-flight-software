/**
 * @author  Claude Code
 * @date    2025-01-10
 *
 * Unit tests for packet authentication module.
 */

#include "packet_test.h"
#include <assert.h>
#include <string.h>

// Mock boot count for testing
static uint32_t mock_boot_count = 42;

void test_packet_structure_layout()
{
    // Test that packet structure has correct layout
    assert(sizeof(packet_t) == 255);
    assert(offsetof(packet_t, hmac) ==
           sizeof(packet_t) - TC_SHA256_DIGEST_SIZE);

    printf("✓ Packet structure layout correct\n");
}

void test_packet_null_pointer()
{
    // Test NULL packet pointer
    bool result = is_packet_authenticated(NULL, mock_boot_count);

#ifdef PACKET_HMAC_PSK
    // With authentication enabled, NULL should return false
    assert(result == false);
    printf("✓ NULL packet pointer rejected when authentication enabled\n");
#else
    // With authentication disabled, all packets pass (including NULL)
    assert(result == true);
    printf("✓ NULL packet pointer accepted when authentication disabled\n");
#endif
}

void test_packet_authentication_disabled()
{
    // Test with authentication disabled (no PACKET_HMAC_PSK)
    // This test assumes PACKET_HMAC_PSK is NOT defined
    packet_t packet = {0};

    // Should always return true when auth is disabled
    bool result = is_packet_authenticated(&packet, mock_boot_count);
#ifndef PACKET_HMAC_PSK
    assert(result == true);
    printf("✓ Authentication disabled mode works\n");
#else
    // With authentication enabled, empty packet should fail
    assert(result == false);
    printf("✓ Authentication enabled mode works\n");
#endif
}

void test_packet_header_constants()
{
    // Test that packet size constants are correct
    assert(PACKET_SIZE == 255);
    assert(PACKET_HEADER_SIZE == 5);  // dst, src, flags, seq, len
    assert(PACKET_HMAC_SIZE == 32);   // SHA256 digest size
    assert(PACKET_FOOTER_SIZE == 40); // boot_count(4) + msg_id(4) + hmac(32)

    printf("✓ Packet constants correct\n");
}

void test_packet_data_size()
{
    // Test that data size calculation is correct
    packet_t packet;
    size_t expected_data_size =
        255 - 5 - 8 - 32; // total - header - footer_no_hmac - hmac
    assert(sizeof(packet.data) == expected_data_size);

    printf("✓ Packet data size correct\n");
}

#ifdef PACKET_HMAC_PSK
void test_replay_protection_boot_count()
{
    packet_t packet = {0};
    packet.boot_count = mock_boot_count + 1; // Wrong boot count
    packet.msg_id = 1;

    bool result = is_packet_authenticated(&packet, mock_boot_count);
    assert(result == false);

    printf("✓ Boot count mismatch detection works\n");
}

void test_replay_protection_msg_id()
{
    packet_t packet = {0};
    packet.boot_count = mock_boot_count;
    packet.msg_id = 0; // msg_id must be > last_seen_msg_id

    bool result = is_packet_authenticated(&packet, mock_boot_count);
    assert(result == false);

    printf("✓ Message ID replay protection works\n");
}
#endif

int main()
{
    printf("Starting packet authentication tests...\n");

    test_packet_structure_layout();
    test_packet_header_constants();
    test_packet_data_size();
    test_packet_null_pointer();
    test_packet_authentication_disabled();

#ifdef PACKET_HMAC_PSK
    test_replay_protection_boot_count();
    test_replay_protection_msg_id();
    printf(
        "Note: HMAC authentication tests run with PACKET_HMAC_PSK defined\n");
#else
    printf(
        "Note: HMAC authentication disabled (PACKET_HMAC_PSK not defined)\n");
#endif

    printf("All packet authentication tests passed!\n");
    return 0;
}