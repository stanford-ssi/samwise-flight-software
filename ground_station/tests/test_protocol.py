"""
Protocol tests for ground station communication.
Byte formats must remain in sync with the Kaitai Struct specification:
@see https://github.com/stanford-ssi/samwise-flight-software/blob/main/ground_station/samwise.ksy
"""

import os
import struct
import sys

import pytest

# Add project root to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "../.."))

# Import ground_station modules as a package (hardware mocking handled in conftest.py)
from ground_station import protocol
from ground_station import state as state_module


@pytest.fixture
def test_state():
    """Create a temporary state manager for testing"""
    # Create runtime_artifacts directory if it doesn't exist
    runtime_dir = os.path.join(os.path.dirname(__file__), "runtime_artifacts")
    os.makedirs(runtime_dir, exist_ok=True)

    # Use a temporary state file for tests to avoid polluting production data
    test_state_file = os.path.join(runtime_dir, "test_gs_state.json")
    test_state_manager = state_module.StateManager(state_file=test_state_file)

    # Inject our test state into the global state_manager for protocol to use
    original_state_manager = state_module.state_manager
    state_module.state_manager = test_state_manager

    # Reset state headers for deterministic tests
    test_state_manager.boot_count = 100
    test_state_manager.msg_id = 50

    yield test_state_manager

    # Restore original state manager after test
    state_module.state_manager = original_state_manager

    # Clean up test state file
    try:
        os.remove(test_state_file)
    except (OSError, FileNotFoundError):
        pass


@pytest.mark.unit
@pytest.mark.protocol
def test_create_cmd_payload():
    """Test command payload creation with string and bytes"""
    # Test string payload
    payload = protocol.create_cmd_payload(0x01, "test")
    assert payload[0] == 0x01
    assert payload[1:] == b"test"

    # Test bytes payload
    payload = protocol.create_cmd_payload(0x02, b"\xDE\xAD")  # fmt: skip
    assert payload[0] == 0x02
    assert payload[1:] == b"\xDE\xAD"  # fmt: skip


@pytest.mark.unit
@pytest.mark.protocol
@pytest.mark.skip(
    reason="State fixture injection needs fixing - state_manager reference timing issue"
)
def test_packet_creation_structure(test_state):
    """Test packet creation structure matches protocol specification"""
    builder = protocol.LegacyPacketBuilder()
    data = b"hello"

    # create_packet(dst, src, flags, seq, data)
    packet = builder.create_packet(1, 2, 0, 0, data)

    # Structure:
    # Header (5 bytes): dst, src, flags, seq, len
    # Data (len bytes)
    # Footer (8 bytes): boot_count(4), msg_id(4)
    # HMAC (32 bytes)

    expected_len = 5 + len(data) + 8 + 32
    assert len(packet) == expected_len

    # Verify Header
    assert packet[0] == 1  # dst
    assert packet[1] == 2  # src
    assert packet[4] == len(data)  # len

    # Verify Data
    assert packet[5 : 5 + len(data)] == data

    # Verify Footer (Boot Count & Msg ID)
    footer_start = 5 + len(data)
    boot_count, msg_id = struct.unpack("<II", packet[footer_start : footer_start + 8])

    assert boot_count == 100
    assert msg_id == 51  # 50 + 1 (incremented)


@pytest.mark.unit
@pytest.mark.protocol
def test_beacon_decode():
    """Test beacon packet decoding matches C struct layout"""
    # Construct a fake beacon packet matching the C struct layout
    # Format: data_len (1 byte) + debug_string (null terminated) + stats struct + callsign

    # struct beacon_stats (all little endian)
    # uint32_t reboot_counter;
    # uint64_t time_in_state_ms;
    # uint32_t rx_bytes;
    # uint32_t rx_packets;
    # uint32_t rx_backpressure_drops;
    # uint32_t rx_bad_packet_drops;
    # uint32_t tx_bytes;
    # uint32_t tx_packets;
    # uint16_t battery_voltage_mv;
    # uint16_t battery_current_ma;
    # uint16_t solar_voltage_mv;
    # uint16_t solar_current_ma;
    # uint16_t panel_A_voltage_mv;
    # uint16_t panel_A_current_ma;
    # uint16_t panel_B_voltage_mv;
    # uint16_t panel_B_current_ma;
    # uint8_t device_status;

    # Hex dump of a sample beacon packet
    # Format: debug_string (null-terminated) + beacon_stats (53 bytes) + adcs (25 bytes) + callsign (6 bytes)
    # This packet has: reboot_counter=42, battery_voltage=4000
    # The following packet is printed from //src/tasks/beacon/test/beacon_test.c with the current beacon_task.c implementation
    beacon_packet_hex = """
6d 6f 63 6b 5f 73 74 61 74 65
20 62 65 61 74 20 63 61 6c 21
00 2a 00 00 00 39 30 00 00 00
00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 a0 0f 00
00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 80 3f cd cc
cc 3d cd cc 4c 3e 9a 99 99 3e
cd cc cc 3e 41 2a 00 00 00 4b
43 33 57 4e 59 00
"""
    beacon_packet_hex_clean = beacon_packet_hex.replace(" ", "").replace("\n", "")
    beacon_packet_bytes = bytes.fromhex(beacon_packet_hex_clean)
    beacon_packet = bytes([len(beacon_packet_bytes)]) + beacon_packet_bytes

    result = protocol.decode_beacon_data(beacon_packet)

    assert result.state_name == "mock_state beat cal!"
    assert result.stats.reboot_counter == 42
    assert result.stats.battery_voltage == 4000
    assert result.callsign == "KC3WNY"


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
