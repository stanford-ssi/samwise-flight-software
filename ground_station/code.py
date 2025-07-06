import board
import busio
import digitalio
import time
import random
import struct
from adafruit_hashlib import sha256
import circuitpython_hmac as hmac
import sys


def detect_board():
    """Detect which board we're running on"""
    # Check for Pico 2 specific pins
    if hasattr(board, 'GP0') and hasattr(board, 'GP28'):
        return "PICO2"
    # Check for Feather M4 specific pins
    elif hasattr(board, 'D0') and hasattr(board, 'SCK'):
        return "FEATHER_M4"
    else:
        # Default fallback
        return "UNKNOWN"

def get_board_pins(board_type):
    """Get pin configuration for specific board"""
    if board_type == "PICO2":
        return {
            'MOSI': board.GP15,
            'MISO': board.GP12,
            'SCK': board.GP14,
            'CS': board.GP13,
            'RESET': board.GP11,
            'LED': board.LED
        }
    elif board_type == "FEATHER_M4":
        return {
            'MOSI': board.MOSI,     # Usually D23
            'MISO': board.MISO,     # Usually D22
            'SCK': board.SCK,       # Usually D24
            'CS': board.A5,         # Analog pin 5
            'RESET': board.D5,      # Digital pin 5
            'LED': board.D13        # Built-in LED
        }
    else:
        # Default to Pico pins
        print("Unknown board, defaulting to Pico 2 pin configuration")
        return {
            'MOSI': board.GP15,
            'MISO': board.GP12,
            'SCK': board.GP14,
            'CS': board.GP13,
            'RESET': board.GP11,
            'LED': board.LED
        }

# Detect board and configure pins
BOARD_TYPE = detect_board()
print(f"Detected board: {BOARD_TYPE}")

pins = get_board_pins(BOARD_TYPE)
MOSI_PIN = pins['MOSI']
MISO_PIN = pins['MISO']
SCK_PIN = pins['SCK']
CS_PIN = pins['CS']
RESET_PIN = pins['RESET']
LED_PIN = pins['LED']

print(f"Pin configuration:")
print(f"  MOSI: {MOSI_PIN}")
print(f"  MISO: {MISO_PIN}")
print(f"  SCK: {SCK_PIN}")
print(f"  CS: {CS_PIN}")
print(f"  RESET: {RESET_PIN}")

led = digitalio.DigitalInOut(LED_PIN)
led.direction = digitalio.Direction.OUTPUT

spi = busio.SPI(SCK_PIN, MOSI=MOSI_PIN, MISO=MISO_PIN)
cs = digitalio.DigitalInOut(CS_PIN)
reset = digitalio.DigitalInOut(RESET_PIN)

# Default packet constants
DEFAULT_PACKET_HMAC_PSK = b"0M09De7LOHdzMVPIYpYo4NsFOI9rTUz1"
DEFAULT_BOOT_COUNT = 474
STARTING_MSG_ID = 1
PACKET_HEADER_SIZE = 5  # dst, src, flags, seq, len
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

rfm9x = None


class PacketBuilder:
    def __init__(self):
        self.msg_id = STARTING_MSG_ID

    def create_packet(self, dst, src, flags, seq, data):
        if len(data) > PACKET_MAX_DATA_SIZE:
            raise ValueError(
                f"Data too large: {len(data)} bytes (max {PACKET_MAX_DATA_SIZE})"
            )

        # Pack header fields
        packet = struct.pack("BBBBB", dst, src, flags, seq, len(data))

        print(f"Data length: {len(data)}, Data: {data}")

        # Add data
        packet += data

        # Add boot count and msg_id (flight software always expects these)
        packet += struct.pack("<II", config['boot_count'], self.msg_id)

        # Flight software always expects HMAC authentication
        # Calculate HMAC (even if auth is "disabled", we still send it for compatibility)
        h = hmac.new(config['packet_hmac_psk'], msg=packet, digestmod=sha256)
        packet += h.digest()

        # Debug packet contents
        print(f"Total packet size: {len(packet)} bytes (min required: 45)")
        for i, byte in enumerate(packet):
            print(f"[DEBUG] {i:02d}: {byte:02x}")

        self.msg_id += 1
        
        # RFM9x library adds 4-byte header, so we return the full packet
        # The flight software expects the complete packet structure
        return packet

    def unpack_packet(self, packet_bytes):
        if len(packet_bytes) < PACKET_HEADER_SIZE:
            raise ValueError("Packet too small")

        # Unpack header
        dst, src, flags, seq, data_len = struct.unpack(
            "BBBBB", packet_bytes[:PACKET_HEADER_SIZE]
        )

        # Extract data
        data = packet_bytes[PACKET_HEADER_SIZE:PACKET_HEADER_SIZE + data_len]

        if config['auth_enabled']:
            # Extract HMAC
            received_hmac = packet_bytes[-PACKET_HMAC_SIZE:]
            packet_without_hmac = packet_bytes[:-PACKET_HMAC_SIZE]

            # Verify HMAC
            h = hmac.new(config['packet_hmac_psk'], packet_without_hmac, sha256)
            if not hmac.compare_digest(h.digest(), received_hmac):
                raise ValueError("HMAC verification failed")

            # Unpack footer
            footer_start = len(packet_bytes) - PACKET_FOOTER_SIZE
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


def try_get_packet(timeout=0.1):
    """Check for incoming packets with short timeout"""
    packet = rfm9x.receive(timeout=timeout)
    if packet is not None:
        try:
            unpacked = packet_builder.unpack_packet(packet)
            print(f"\n>>> PACKET RECEIVED <<<")
            print(f"  Destination: {unpacked['dst']}")
            print(f"  Source: {unpacked['src']}")
            print(f"  Flags: {unpacked['flags']}")
            print(f"  Sequence: {unpacked['seq']}")
            print(f"  Data: {unpacked['data']}")
            if 'boot_count' in unpacked:
                print(f"  Boot Count: {unpacked['boot_count']}")
                print(f"  Message ID: {unpacked['msg_id']}")
            print(">>> END PACKET <<<\n")
            return True
        except Exception as e:
            print(f"\n>>> ERROR unpacking packet: {e} <<<")
            return False
    return False

def get_user_input_with_timeout(prompt, timeout=0.5):
    """Get user input with timeout, checking for packets while waiting"""
    import time
    
    print(prompt, end='', flush=True)
    start_time = time.monotonic()
    user_input = ""
    
    while True:
        # Check for packets every 100ms
        if try_get_packet(timeout=0.1):
            # Packet received, redisplay prompt
            print(f"\r{prompt}", end='', flush=True)
        
        # Simple timeout mechanism (CircuitPython doesn't have select)
        if time.monotonic() - start_time > timeout:
            break
            
        time.sleep(0.1)
    
    return None


def create_cmd_payload(cmd_id, cmd_payload=""):
    if isinstance(cmd_payload, str):
        return bytes([cmd_id] + list(cmd_payload.encode("utf-8")))
    else:
        return bytes([cmd_id]) + cmd_payload


def send_command(cmd_id, cmd_payload="", dst=0xFF):
    """Send a command packet to the flight software"""
    data = create_cmd_payload(cmd_id, cmd_payload)
    packet = packet_builder.create_packet(
        dst=dst,  # Destination address
        src=0xFF,  # Source address (ground station)
        flags=0x00,  # No special flags
        seq=0x00,  # Sequence number
        data=data,  # Command data
    )
    print(f"Packet dump: " + " ".join(f"{byte:02x}" for byte in packet))
    rfm9x.send(packet)
    print(f"Sent packet with message ID: {packet_builder.msg_id - 1}, {len(packet) + 4=}")

def send_no_op():
    """Send a NO_OP ping command"""
    send_command(NO_OP)
    
def send_payload_exec(command_str):
    """Send a payload execution command"""
    send_command(PAYLOAD_EXEC, command_str)
    
def send_payload_turn_on():
    """Send payload turn on command"""
    send_command(PAYLOAD_TURN_ON)
    
def send_payload_turn_off():
    """Send payload turn off command"""
    send_command(PAYLOAD_TURN_OFF)
    
def send_manual_state_override(state_name):
    """Send manual state override command"""
    send_command(MANUAL_STATE_OVERRIDE, state_name)


def get_user_input(prompt, default=None):
    """Get user input with optional default value"""
    if default is not None:
        full_prompt = f"{prompt} [{default}]: "
    else:
        full_prompt = f"{prompt}: "
    
    try:
        response = input(full_prompt).strip()
        if not response and default is not None:
            return str(default)
        return response
    except KeyboardInterrupt:
        print("\nExiting...")
        sys.exit()

def get_yes_no(prompt, default=True):
    """Get yes/no input from user"""
    default_str = "Y/n" if default else "y/N"
    response = get_user_input(f"{prompt} ({default_str})", "y" if default else "n")
    return response.lower() in ['y', 'yes', '1', 'true']

def configure_authentication():
    """Interactive authentication configuration"""
    print("\n=== Authentication Configuration ===")
    config['auth_enabled'] = get_yes_no("Enable HMAC authentication?", True)
    
    if config['auth_enabled']:
        new_psk = get_user_input("Enter PSK (leave blank for default)", None)
        if new_psk:
            config['packet_hmac_psk'] = new_psk.encode('utf-8')
        
        boot_count_str = get_user_input("Enter boot count", config['boot_count'])
        try:
            config['boot_count'] = int(boot_count_str)
        except ValueError:
            print("Invalid boot count, using default")
    else:
        print("Authentication disabled - packets will not be authenticated")

def configure_lora_settings():
    """Interactive LoRA configuration"""
    print("\n=== LoRA Configuration ===")
    print(f"Current settings match flight software defaults:")
    print(f"  Frequency: {config['frequency']} MHz")
    print(f"  Bandwidth: {config['bandwidth']} Hz")
    print(f"  Spreading Factor: {config['spreading_factor']}")
    print(f"  Coding Rate: {config['coding_rate']}")
    print(f"  CRC: {config['crc']}")
    
    if get_yes_no("Modify LoRA settings?", False):
        freq_str = get_user_input("Frequency (MHz)", config['frequency'])
        try:
            config['frequency'] = float(freq_str)
        except ValueError:
            print("Invalid frequency, using default")
        
        bw_str = get_user_input("Bandwidth (Hz)", config['bandwidth'])
        try:
            config['bandwidth'] = int(bw_str)
        except ValueError:
            print("Invalid bandwidth, using default")
        
        sf_str = get_user_input("Spreading Factor (6-12)", config['spreading_factor'])
        try:
            sf = int(sf_str)
            if 6 <= sf <= 12:
                config['spreading_factor'] = sf
            else:
                print("Spreading factor must be 6-12, using default")
        except ValueError:
            print("Invalid spreading factor, using default")
        
        cr_str = get_user_input("Coding Rate (5-8)", config['coding_rate'])
        try:
            cr = int(cr_str)
            if 5 <= cr <= 8:
                config['coding_rate'] = cr
            else:
                print("Coding rate must be 5-8, using default")
        except ValueError:
            print("Invalid coding rate, using default")
        
        config['crc'] = get_yes_no("Enable CRC?", config['crc'])

def initialize_radio():
    """Initialize the RFM9x radio with configured settings"""
    global rfm9x
    import adafruit_rfm9x
    
    print("\n=== Initializing Radio ===")
    print(f"Frequency: {config['frequency']} MHz")
    print(f"Bandwidth: {config['bandwidth']} Hz")
    print(f"Spreading Factor: {config['spreading_factor']}")
    print(f"Coding Rate: {config['coding_rate']}")
    print(f"CRC: {config['crc']}")
    
    rfm9x = adafruit_rfm9x.RFM9x(spi, cs, reset, config['frequency'], crc=config['crc'])
    rfm9x.signal_bandwidth = config['bandwidth']
    rfm9x.spreading_factor = config['spreading_factor']
    rfm9x.coding_rate = config['coding_rate']
    
    print("Radio initialized successfully!")

def show_command_menu():
    """Display available commands"""
    print("\n=== Available Commands ===")
    print("1. Send NO_OP (ping)")
    print("2. Send Payload Execute")
    print("3. Send Payload Turn On")
    print("4. Send Payload Turn Off")
    print("5. Send Manual State Override")
    print("r. Check for received packets")
    print("q. Quit")
    print("h. Show this help")

def interactive_command_loop():
    """Main interactive command loop with continuous packet monitoring"""
    show_command_menu()
    print("\nSystem is now monitoring for packets continuously...")
    print("Press Enter to show command prompt, or type commands when prompted.")
    
    last_prompt_time = time.monotonic()
    prompt_interval = 5.0  # Show prompt every 5 seconds
    
    while True:
        try:
            # Continuously check for packets
            try_get_packet(timeout=0.1)
            
            # Show prompt periodically or when user presses enter
            current_time = time.monotonic()
            if current_time - last_prompt_time > prompt_interval:
                try:
                    cmd = input("\n[Monitoring...] Enter command (h for help, q to quit): ").strip().lower()
                    last_prompt_time = current_time
                    
                    if cmd == 'q':
                        print("Goodbye!")
                        break
                    elif cmd == 'h':
                        show_command_menu()
                    elif cmd == 'r':
                        print("Checking for packets...")
                        try_get_packet(timeout=1.0)
                    elif cmd == '1':
                        print("Sending NO_OP...")
                        send_no_op()
                    elif cmd == '2':
                        payload_cmd = get_user_input("Enter payload command", '["take_photo", ["test1"], {"w": 1024, "h": 768, "cell": 128}]')
                        print(f"Sending payload execute: {payload_cmd}")
                        send_payload_exec(payload_cmd)
                    elif cmd == '3':
                        print("Sending payload turn on...")
                        send_payload_turn_on()
                    elif cmd == '4':
                        print("Sending payload turn off...")
                        send_payload_turn_off()
                    elif cmd == '5':
                        state_name = get_user_input("Enter state name", "running_state")
                        print(f"Sending state override: {state_name}")
                        send_manual_state_override(state_name)
                    elif cmd == '':
                        # Just pressed enter, continue monitoring
                        pass
                    else:
                        print("Unknown command. Type 'h' for help.")
                
                except EOFError:
                    # Handle Ctrl+D
                    print("\nGoodbye!")
                    break
            else:
                # Brief sleep to prevent excessive CPU usage
                time.sleep(0.1)
        
        except KeyboardInterrupt:
            print("\nGoodbye!")
            break
        except Exception as e:
            print(f"Error: {e}")
            time.sleep(0.1)

def main():
    """Main program entry point"""
    print("=== Samwise Ground Station ===")
    print("Interactive LoRA Communication System")
    
    # Interactive configuration
    configure_authentication()
    configure_lora_settings()
    
    # Initialize radio with configured settings
    initialize_radio()
    
    # Create packet builder with current config
    global packet_builder
    packet_builder = PacketBuilder()
    
    print("\n=== Starting Interactive Command Loop ===")
    print("The system will continuously check for incoming packets.")
    print("Type commands to send packets to the flight software.")
    
    # Start interactive command loop
    interactive_command_loop()

if __name__ == "__main__":
    main()
