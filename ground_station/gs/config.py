
# Default packet constants
DEFAULT_PACKET_HMAC_PSK = b"0M09De7LOHdzMVPIYpYo4NsFOI9rTUz1"
DEFAULT_BOOT_COUNT = 474
STARTING_MSG_ID = 1
PACKET_HEADER_SIZE = 1  # len
PACKET_HMAC_SIZE = 32  # SHA256 size
PACKET_FOOTER_SIZE = 8 + PACKET_HMAC_SIZE  # boot_count (4) + msg_id (4) + hmac
PACKET_MAX_DATA_SIZE = 255 - PACKET_HEADER_SIZE - PACKET_FOOTER_SIZE

# Default LoRA settings from flight software
DEFAULT_FREQUENCY = 438.1
DEFAULT_BANDWIDTH = 125000
DEFAULT_SPREADING_FACTOR = 7
DEFAULT_CODING_RATE = 5
DEFAULT_CRC = True

# Command constants (from flight software)
NO_OP = 0
PAYLOAD_EXEC = 1
PAYLOAD_TURN_ON = 2
PAYLOAD_TURN_OFF = 3
MANUAL_STATE_OVERRIDE = 4

# Global configuration variables
config = {
    'auth_enabled': True,
    'packet_hmac_psk': DEFAULT_PACKET_HMAC_PSK,
    'boot_count': DEFAULT_BOOT_COUNT,
    'frequency': DEFAULT_FREQUENCY,
    'bandwidth': DEFAULT_BANDWIDTH,
    'spreading_factor': DEFAULT_SPREADING_FACTOR,
    'coding_rate': DEFAULT_CODING_RATE,
    'crc': DEFAULT_CRC
}
