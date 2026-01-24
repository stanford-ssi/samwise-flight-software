import struct
try:
    from adafruit_hashlib import sha256
except:
    from hashlib import sha256
try:
    import circuitpython_hmac as hmac
except:
    import hmac
from . import config
from .state import state_manager

def create_cmd_payload(cmd_id, cmd_payload=""):
    if isinstance(cmd_payload, str):
        return bytes([cmd_id] + list(cmd_payload.encode("utf-8")))
    else:
        return bytes([cmd_id]) + cmd_payload

def safe_compare_digest(a, b):
    """Constant time comparison to avoid timing attacks"""
    if len(a) != len(b):
        return False
    result = 0
    for x, y in zip(a, b):
        result |= x ^ y
    return result == 0

class PacketBuilder:
    def __init__(self):
        # We no longer maintain internal state, we rely on state_manager
        pass

    def create_packet(self, dst, src, flags, seq, data):
        if len(data) > config.PACKET_MAX_DATA_SIZE:
            raise ValueError(
                f"Data too large: {len(data)} bytes (max {config.PACKET_MAX_DATA_SIZE})"
            )

        # Pack header fields
        packet = struct.pack("BBBBB", dst, src, flags, seq, len(data))

        print("Data length: {}, Data: {}".format(len(data), data))

        # Add data
        packet += data
        
        # Get dynamic state
        current_boot_count = state_manager.boot_count
        next_msg_id = state_manager.get_next_msg_id()

        # Add boot count and msg_id (flight software always expects these)
        packet += struct.pack("<II", current_boot_count, next_msg_id)

        # Flight software always expects HMAC authentication
        # Calculate HMAC (even if auth is "disabled", we still send it for compatibility)
        h = hmac.new(config.config['packet_hmac_psk'], msg=packet, digestmod=sha256)
        packet += h.digest()

        # Debug packet contents
        print("Total packet size: {} bytes (min required: 45)".format(len(packet)))
        for i, byte in enumerate(packet):
            print("[DEBUG] {:02d}: {:02x}".format(i, byte))
        
        # RFM9x library adds 4-byte header, so we return the full packet
        # The flight software expects the complete packet structure
        return packet

    def unpack_packet(self, packet_bytes):
        if len(packet_bytes) < config.PACKET_HEADER_SIZE:
            raise ValueError("Packet too small")

        # Unpack header
        dst, src, flags, seq, data_len = struct.unpack(
            "BBBBB", packet_bytes[:config.PACKET_HEADER_SIZE]
        )

        # Extract data
        data = packet_bytes[config.PACKET_HEADER_SIZE:config.PACKET_HEADER_SIZE + data_len]

        if config.config['auth_enabled']:
            # Extract HMAC
            received_hmac = packet_bytes[-config.PACKET_HMAC_SIZE:]
            packet_without_hmac = packet_bytes[:-config.PACKET_HMAC_SIZE]

            # Verify HMAC
            h = hmac.new(config.config['packet_hmac_psk'], msg=packet_without_hmac, digestmod=sha256)
            if not safe_compare_digest(h.digest(), received_hmac):
               print("HMAC verification failed. Received: {}, Calculated: {}".format(
                   ' '.join('{:02x}'.format(b) for b in received_hmac),
                   ' '.join('{:02x}'.format(b) for b in h.digest())
               ))
               # raise ValueError("HMAC verification failed") # Warning only for now


            # Unpack footer
            footer_start = len(packet_bytes) - config.PACKET_FOOTER_SIZE
            boot_count, msg_id = struct.unpack('<II', packet_bytes[footer_start:footer_start+8])
            
            return {
                "dst": dst,
                "src": src,
                "flags": flags,
                "seq": seq,
                "data": data,
                'boot_count': boot_count,
                'msg_id': msg_id
            }
        else:
            return {
                "dst": dst,
                "src": src,
                "flags": flags,
                "seq": seq,
                "data": data
            }

packet_builder = PacketBuilder()

def decode_beacon_data(data):
    """Decode beacon data payload from beacon_task.c format"""
    try:
        print("DEBUG: decode_beacon_data called with {} bytes".format(len(data)))
        print("DEBUG: data = {}".format(' '.join('{:02x}'.format(b) for b in data)))
        
        # Find the null terminator for the state name
        null_pos = data.find(b'\x00')
        print("DEBUG: null_pos = {}".format(null_pos))
        
        if null_pos == -1:
            # No null terminator found, assume entire data is state name
            print("DEBUG: No null terminator found")
            try:
                state_name = data.decode('utf-8')
            except UnicodeDecodeError:
                # Fallback for invalid UTF-8
                state_name = str(data)
            return {"state_name": state_name, "stats": None}
        
        # Extract state name (null-terminated string)
        try:
            state_name = data[:null_pos].decode('utf-8')
        except UnicodeDecodeError:
            # Fallback for invalid UTF-8
            state_name = str(data[:null_pos])
        print("DEBUG: Decoded state name: '{}'".format(state_name))
        
        # Extract beacon statistics starting after null terminator
        stats_start = null_pos + 1
        print("DEBUG: stats_start = {}, remaining bytes = {}".format(stats_start, len(data) - stats_start))
        
        # New beacon_stats struct size: 4+8+4+4+4+4+4+4+2+2+2+2+1 = 45 bytes
        # Plus ADCS packet size: 25 bytes (float w + 4 floats quaternion + char state + uint32_t boot_count)
        # Total expected: 45 + 25 = 70 bytes after state name
        if len(data) >= stats_start + 45:  # 45 bytes for beacon_stats struct
            print("DEBUG: Extracting stats data...")
            stats_data = data[stats_start:stats_start + 45]
            print("DEBUG: Extracted stats data ({} bytes): {}".format(len(stats_data), ' '.join('{:02x}'.format(b) for b in stats_data)))
            
            print("DEBUG: About to call struct.unpack...")

            print("DEBUG: Boot bytes: {}".format(
                ' '.join('{:02x}'.format(b) for b in stats_data[:4])
            ))

            # Unpack new beacon_stats struct (all little-endian)
            # CircuitPython struct may not support all format codes, let's try simpler ones
            # uint32_t reboot_counter, uint64_t time, 6x uint32_t values, 4x uint16_t values, 1x uint8_t
            # L=uint32, Q=uint64, H=uint16, B=uint8 - try using I for uint32 instead of L
            unpacked_stats = struct.unpack('<LQ6L4HB', stats_data)
            print("DEBUG: struct.unpack successful, got {} values".format(len(unpacked_stats)))
            
            # Q format worked (uint64 for time)
            beacon_stats = {
                "reboot_counter": unpacked_stats[0],
                "time_in_state_ms": unpacked_stats[1],
                "rx_bytes": unpacked_stats[2],
                "rx_packets": unpacked_stats[3],
                "rx_backpressure_drops": unpacked_stats[4],
                "rx_bad_packet_drops": unpacked_stats[5],
                "tx_bytes": unpacked_stats[6],
                "tx_packets": unpacked_stats[7],
                "battery_voltage": unpacked_stats[8],
                "battery_current": unpacked_stats[9],
                "solar_voltage": unpacked_stats[10],
                "solar_current": unpacked_stats[11],
                "device_status": unpacked_stats[12]
            }
            
            # Check if ADCS data is appended after beacon stats
            adcs_start = stats_start + 45
            adcs_data = None
            if len(data) >= adcs_start + 25:  # 25 bytes for ADCS packet
                print("DEBUG: ADCS data detected, extracting...")
                adcs_payload = data[adcs_start:adcs_start + 25]
                adcs_data = decode_adcs_data(adcs_payload)
                if 'error' not in adcs_data:
                    print("DEBUG: ADCS decode successful")
                else:
                    print("DEBUG: ADCS decode failed: {}".format(adcs_data.get('error', 'unknown')))
                    adcs_data = None
            else:
                print("DEBUG: No ADCS data present (need {} bytes, have {})".format(25, len(data) - adcs_start))
            
            return {
                "state_name": state_name,
                "stats": beacon_stats,
                "adcs": adcs_data
            }
        else:
            print("DEBUG: Not enough data for stats, need {} bytes, have {}".format(45, len(data) - stats_start))
            return {"state_name": state_name, "stats": None}
            
    except Exception as e:
        print("[decode_beacon_data] Error decoding beacon data: {}".format(e))
        print("DEBUG: Full traceback:")
        return {"state_name": "decode_error", "stats": None}

def decode_adcs_data(data):
    """Decode ADCS telemetry data from adcs_packet.h format"""
    try:
        print("DEBUG: decode_adcs_data called with {} bytes".format(len(data)))
        print("DEBUG: data = {}".format(' '.join('{:02x}'.format(b) for b in data)))
        
        # ADCS packet structure from adcs_packet.h:
        # float w (4 bytes) - Angular velocity
        # float q0, q1, q2, q3 (16 bytes) - Quaternion estimate
        # char state (1 byte) - State
        # uint32_t boot_count (4 bytes) - Boot count
        # Total: 25 bytes
        
        if len(data) < 25:
            print("DEBUG: ADCS packet too short, need 25 bytes, got {}".format(len(data)))
            return {"error": "packet_too_short", "expected": 25, "actual": len(data)}
        
        # Unpack ADCS data (little-endian floats, char, uint32)
        # f = float (4 bytes), B = unsigned char (1 byte), L = uint32 (4 bytes)
        unpacked = struct.unpack('<fffffBL', data[:25])
        
        adcs_data = {
            "angular_velocity": unpacked[0],
            "quaternion": {
                "q0": unpacked[1],
                "q1": unpacked[2], 
                "q2": unpacked[3],
                "q3": unpacked[4]
            },
            "state": unpacked[5],
            "boot_count": unpacked[6]
        }
        
        print("DEBUG: ADCS decode successful")
        return adcs_data
        
    except Exception as e:
        print("[decode_adcs_data] Error decoding ADCS data: {}".format(e))
        return {"error": "decode_failed", "exception": str(e)}
