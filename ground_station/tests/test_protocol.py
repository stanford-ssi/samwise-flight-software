import sys
import os
import struct
import unittest

# Add parent directory to path so we can import 'gs' package
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

# Mock hardware libraries BEFORE importing gs
from unittest.mock import MagicMock
sys.modules['board'] = MagicMock()
sys.modules['busio'] = MagicMock()
sys.modules['digitalio'] = MagicMock()
sys.modules['adafruit_rfm9x'] = MagicMock()

from gs.protocol import PacketBuilder, create_cmd_payload, decode_beacon_data
from gs.state import state_manager
from gs import config

class TestProtocol(unittest.TestCase):
    def setUp(self):
        # Reset state headers for deterministic tests
        state_manager.boot_count = 100
        state_manager.msg_id = 50

    def test_create_cmd_payload(self):
        # Test string payload
        payload = create_cmd_payload(0x01, "test")
        self.assertEqual(payload[0], 0x01)
        self.assertEqual(payload[1:], b"test")
        
        # Test bytes payload
        payload = create_cmd_payload(0x02, b"\xDE\xAD")
        self.assertEqual(payload[0], 0x02)
        self.assertEqual(payload[1:], b"\xDE\xAD")

    def test_packet_creation_structure(self):
        builder = PacketBuilder()
        data = b"hello"
        
        # create_packet(dst, src, flags, seq, data)
        packet = builder.create_packet(1, 2, 0, 0, data)
        
        # Structure:
        # Header (5 bytes): dst, src, flags, seq, len
        # Data (len bytes)
        # Footer (8 bytes): boot_count(4), msg_id(4)
        # HMAC (32 bytes)
        
        expected_len = 5 + len(data) + 8 + 32
        self.assertEqual(len(packet), expected_len)
        
        # Verify Header
        self.assertEqual(packet[0], 1) # dst
        self.assertEqual(packet[1], 2) # src
        self.assertEqual(packet[4], len(data)) # len
        
        # Verify Data
        self.assertEqual(packet[5:5+len(data)], data)
        
        # Verify Footer (Boot Count & Msg ID)
        footer_start = 5 + len(data)
        boot_count, msg_id = struct.unpack("<II", packet[footer_start:footer_start+8])
        
        self.assertEqual(boot_count, 100)
        self.assertEqual(msg_id, 51) # 50 + 1 (incremented)

    def test_dummy_beacon_decode(self):
        # Construct a fake beacon packet matching the C struct layout
        # state_name (null terminated) + stats struct
        
        state_name = b"idle_state\x00"
        
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
        # uint8_t device_status;
        
        stats_format = '<LQ6L4HB'
        stats_data = struct.pack(stats_format, 
            42, # reboots
            10000, # time
            100, 10, 0, 0, # RX stats
            200, 20, # TX stats
            4000, 100, # Battery
            5000, 500, # Solar
            0x01 # Status
        )
        
        full_payload = state_name + stats_data
        
        result = decode_beacon_data(full_payload)
        
        self.assertEqual(result['state_name'], "idle_state")
        self.assertEqual(result['stats']['reboot_counter'], 42)
        self.assertEqual(result['stats']['battery_voltage'], 4000)

if __name__ == '__main__':
    unittest.main()
