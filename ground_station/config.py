# Default packet constants - Must match flight software protocol implementation
# Flight Software References:
# - Packet Structure: src/tasks/radio/radio_task.c (packet parsing functions)
# - HMAC Key: src/security/auth.h (PACKET_AUTH_KEY)
# - Boot Counter: src/slate/slate.h (slate_t.boot_count)
DEFAULT_PACKET_HMAC_PSK = b"0M09De7LOHdzMVPIYpYo4NsFOI9rTUz1"  # src/security/auth.h:PACKET_AUTH_KEY
DEFAULT_BOOT_COUNT = 474  # Initial sync value - updates from beacon
STARTING_MSG_ID = 1  # Message sequence starts from 1
PACKET_HEADER_SIZE = 5  # src/tasks/radio/radio_task.c: dst, src, flags, seq, len
PACKET_HMAC_SIZE = 32  # SHA256 digest size (src/security/auth.c)
PACKET_FOOTER_SIZE = 8 + PACKET_HMAC_SIZE  # boot_count (4) + msg_id (4) + hmac
PACKET_MAX_DATA_SIZE = 255 - PACKET_HEADER_SIZE - PACKET_FOOTER_SIZE  # RFM9x max payload

# Default LoRA settings - Must match flight software configuration
# Flight Software References:
# - Frequency/Bandwidth: src/drivers/rfm9x/rfm9x.h (RFM9X_FREQUENCY, RFM9X_BANDWIDTH)
# - Spreading Factor/Coding Rate: src/drivers/rfm9x/rfm9x.c:rfm9x_init()
# - CRC Settings: src/drivers/rfm9x/rfm9x.c (rfm9x_set_crc_on)
DEFAULT_FREQUENCY = 438.1  # src/drivers/rfm9x/rfm9x.h:RFM9X_FREQUENCY
DEFAULT_BANDWIDTH = 125000  # src/drivers/rfm9x/rfm9x.h:RFM9X_BANDWIDTH
DEFAULT_SPREADING_FACTOR = 7  # src/drivers/rfm9x/rfm9x.c:rfm9x_init() default SF
DEFAULT_CODING_RATE = 5  # src/drivers/rfm9x/rfm9x.c:rfm9x_init() default CR
DEFAULT_CRC = True  # src/drivers/rfm9x/rfm9x.c:rfm9x_set_crc_on()

# Command constants - Must match flight software command handlers
# Flight Software References:
# - Command IDs: src/tasks/command/command_parser.h (command_id_t enum)
# - Command Processing: src/tasks/command/command_parser.c (parse_command function)
# - State Machine: src/tasks/beacon/beacon_task.c (handles state transitions)
NO_OP = 0  # src/tasks/command/command_parser.h:CMD_NO_OP
PAYLOAD_EXEC = 1  # src/tasks/command/command_parser.h:CMD_PAYLOAD_EXEC
PAYLOAD_TURN_ON = 2  # src/tasks/command/command_parser.h:CMD_PAYLOAD_TURN_ON
PAYLOAD_TURN_OFF = 3  # src/tasks/command/command_parser.h:CMD_PAYLOAD_TURN_OFF
MANUAL_STATE_OVERRIDE = 4  # src/tasks/command/command_parser.h:CMD_MANUAL_STATE_OVERRIDE
PAYLOAD_SHUTDOWN = 5  # src/tasks/command/command_parser.h:CMD_PAYLOAD_SHUTDOWN
ADCS_EXEC = 6
ADCS_PACKET = 7

# Packet filtering configuration
# These filters help reject noisy packets not from the satellite
RSSI_THRESHOLD = -120  # Minimum signal strength in dBm (packets below this are dropped)
EXPECTED_CALLSIGN = "KC3WNY"  # Expected amateur radio callsign suffix (FCC license for Samwise)
ENABLE_RSSI_FILTER = True  # Enable/disable RSSI-based filtering
ENABLE_CALLSIGN_FILTER = True  # Enable/disable callsign verification

# Global configuration variables
config = {
    "auth_enabled": True,
    "packet_hmac_psk": DEFAULT_PACKET_HMAC_PSK,
    "boot_count": DEFAULT_BOOT_COUNT,
    "frequency": DEFAULT_FREQUENCY,
    "bandwidth": DEFAULT_BANDWIDTH,
    "spreading_factor": DEFAULT_SPREADING_FACTOR,
    "coding_rate": DEFAULT_CODING_RATE,
    "crc": DEFAULT_CRC,
    # Packet filtering
    "rssi_threshold": RSSI_THRESHOLD,
    "expected_callsign": EXPECTED_CALLSIGN,
    "enable_rssi_filter": ENABLE_RSSI_FILTER,
    "enable_callsign_filter": ENABLE_CALLSIGN_FILTER,
}
