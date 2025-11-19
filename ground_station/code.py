import board
import busio
import digitalio
import time
import struct
import sys
import supervisor
import adafruit_rfm9x
from adafruit_hashlib import sha256
import circuitpython_hmac as hmac

# ==========================================
# CONFIGURATION
# ==========================================

CONFIG = {
    'frequency': 438.1,
    'bandwidth': 125000,
    'spreading_factor': 7,
    'coding_rate': 5,
    'crc': True,
    'hmac_psk': b"0M09De7LOHdzMVPIYpYo4NsFOI9rTUz1",
    'start_boot_count': 474
}

COMMANDS = {
    'NO_OP': 0,
    'PAYLOAD_EXEC': 1,
    'PAYLOAD_TURN_ON': 2,
    'PAYLOAD_TURN_OFF': 3,
    'MANUAL_STATE_OVERRIDE': 4
}

# ==========================================
# PACKET HANDLER (PROTOCOL LAYER)
# ==========================================

class PacketHandler:
    """Handles packing, unpacking, and HMAC authentication of packets."""
    
    def __init__(self, key):
        self.key = key
        self.msg_id = 1

    def create_packet(self, dst, src, flags, seq, data, boot_count):
        """Builds a full packet with Header, Data, Footer, and HMAC."""
        # 1. Header (RadioHead compatible fields + Length)
        # Format: DST(B), SRC(B), FLAGS(B), SEQ(B), LEN(B)
        header = struct.pack("BBBBB", dst, src, flags, seq, len(data))
        
        # 2. Footer (Metadata)
        # Format: Boot Count (I), Msg ID (I)
        footer = struct.pack("<II", boot_count, self.msg_id)
        
        # 3. Calculate HMAC
        # Signature covers: Header + Data + Footer
        raw_packet = header + data + footer
        digest = hmac.new(self.key, msg=raw_packet, digestmod=sha256).digest()
        
        self.msg_id += 1
        
        # Return complete packet
        return raw_packet + digest

    def parse_beacon(self, packet_bytes):
        """Parses incoming bytes into a dictionary. Returns None if invalid."""
        if len(packet_bytes) < 1:
            return None
        
        # 1. Extract Length
        # Note: RadioHead lib strips the first 4 header bytes (Dst, Src, Id, Flags)
        # So packet_bytes[0] is our custom 'LEN' byte.
        data_len = packet_bytes[0]
        
        if len(packet_bytes) < 1 + data_len:
            return {"error": "Packet smaller than declared length"}

        # 2. Extract Payload
        payload = packet_bytes[1:1+data_len]
        
        # 3. Separate State Name from Struct Data
        # State name is a null-terminated string at the start of payload
        try:
            null_index = payload.find(b'\x00')
            if null_index == -1:
                state_name = str(payload)
                struct_data = b''
            else:
                state_name = payload[:null_index].decode('utf-8', errors='ignore')
                struct_data = payload[null_index+1:]
        except Exception:
            return {"error": "UTF-8 Decode Error"}

        result = {"state": state_name, "stats": None, "adcs": None}

        # 4. Unpack Beacon Stats (45 bytes)
        # Format: L=uint32, Q=uint64, H=uint16, B=uint8
        if len(struct_data) >= 45:
            s = struct.unpack('<LQ6L4HB', struct_data[:45])
            result['stats'] = {
                "reboot_counter": s[0],
                "time_ms": s[1],
                "rx_packets": s[3],
                "tx_packets": s[7],
                "battery_mv": s[8],
                "solar_mv": s[10],
                "status_flags": s[12]
            }
            
            # 5. Unpack ADCS Data (Optional, appended 25 bytes)
            if len(struct_data) >= 45 + 25:
                a = struct.unpack('<fffffBL', struct_data[45:70])
                result['adcs'] = {
                    "w": a[0],
                    "q": (a[1], a[2], a[3], a[4]),
                    "state": a[5],
                    "boot": a[6]
                }
                
        return result

# ==========================================
# GROUND STATION (LOGIC LAYER)
# ==========================================

class GroundStation:
    def __init__(self):
        print("Initializing Ground Station...")
        self.init_hardware()
        self.packet_handler = PacketHandler(CONFIG['hmac_psk'])
        self.boot_count = CONFIG['start_boot_count']
        
        # Buffer for non-blocking input
        self.input_buffer = ""
        
        print(f"Ready. Frequency: {CONFIG['frequency']}MHz")
        self.print_menu()
        print("\n> ", end="")

    def init_hardware(self):
        """Sets up SPI, Pins, and Radio module."""
        # Detect Board Pins
        if hasattr(board, 'GP0'): # Raspberry Pi Pico
            sck, mosi, miso = board.GP14, board.GP15, board.GP12
            cs, reset = board.GP13, board.GP11
            led_pin = board.LED
        else: # Feather M4 / Standard
            sck, mosi, miso = board.SCK, board.MOSI, board.MISO
            cs, reset = board.A5, board.D5
            led_pin = board.D13

        self.led = digitalio.DigitalInOut(led_pin)
        self.led.direction = digitalio.Direction.OUTPUT
        
        spi = busio.SPI(sck, MOSI=mosi, MISO=miso)
        cs_pin = digitalio.DigitalInOut(cs)
        reset_pin = digitalio.DigitalInOut(reset)
        
        self.radio = adafruit_rfm9x.RFM9x(
            spi, cs_pin, reset_pin, CONFIG['frequency'], crc=CONFIG['crc']
        )
        self.radio.signal_bandwidth = CONFIG['bandwidth']
        self.radio.spreading_factor = CONFIG['spreading_factor']
        self.radio.coding_rate = CONFIG['coding_rate']

    def print_menu(self):
        print("\n--- AVAILABLE COMMANDS ---")
        print("1: Ping (NO_OP)")
        print("2: Execute Payload (Photo)")
        print("3: Payload ON")
        print("4: Payload OFF")
        print("5: Manual State Override")
        print("6: SIMULATE Incoming Packet (Test Mode)")

    def send_command(self, cmd_id, payload_str=""):
        """Encapsulates data and sends via Radio."""
        print(f"Sending Command ID: {cmd_id}...")
        
        # Convert payload to bytes
        if isinstance(payload_str, str):
            data = bytes([cmd_id]) + payload_str.encode('utf-8')
        else:
            data = bytes([cmd_id]) + payload_str
            
        # Build Packet
        full_packet = self.packet_handler.create_packet(
            dst=0xFF, src=0xFF, flags=0x00, seq=0x00, 
            data=data, boot_count=self.boot_count
        )
        
        # Debug: Show HMAC
        hmac_sig = full_packet[-32:]
        print(f"TX HMAC: {hmac_sig.hex()}")
        
        # Send (Strip first 4 bytes as RFM9x lib handles Header)
        self.radio.send(full_packet[4:], destination=0xFF, node=0x00, identifier=0x00, flags=0x00)

    def process_incoming_packet(self, packet):
        """Decodes packet, updates state, and prints telemetry."""
        self.led.value = True
        
        # 1. HMAC Check (Visual)
        if len(packet) > 32:
            print(f"\n[RX] RSSI: {self.radio.last_rssi} | HMAC: {packet[-32:].hex()}")
        else:
            print(f"\n[RX] RSSI: {self.radio.last_rssi} | HMAC: Missing/Short")

        # 2. Decode
        data = self.packet_handler.parse_beacon(packet)
        
        if not data or "error" in data:
            print(f"Decode Error: {data.get('error') if data else 'Unknown'}")
            self.led.value = False
            return

        print(f"Sat State: {data['state']}")
        
        if data['stats']:
            stats = data['stats']
            print(f"Telemetry: {stats['battery_mv']}mV | RX: {stats['rx_packets']} | TX: {stats['tx_packets']}")
            
            # 3. Auto-Update Boot Count
            if stats['reboot_counter'] > self.boot_count:
                print(f"*** BOOT COUNT SYNC *** {self.boot_count} -> {stats['reboot_counter']}")
                self.boot_count = stats['reboot_counter']

        if data['adcs']:
            print(f"ADCS State: {data['adcs']['state']} | Q: {data['adcs']['q']}")

        self.led.value = False

    def simulate_packet(self):
        """Generates a fake packet to test local decoding and logic."""
        print("\nSimulating Beacon...")
        # Create fake state and stats
        payload = b"sim_mode\x00" + struct.pack('<LQ6L4HB', 
            self.boot_count + 5, # Force a boot count update
            0, 0,0,0,0,0,0, 4200, 0, 0, 0, 0)
        
        # Wrap in valid packet structure
        full_packet = self.packet_handler.create_packet(
            dst=0xFF, src=0x00, flags=0, seq=0, 
            data=payload, boot_count=self.boot_count + 5
        )
        
        # Feed into receiver (skipping header)
        self.process_incoming_packet(full_packet[4:])

    def handle_user_input(self, cmd_str):
        """Executes the command string from the user."""
        if not cmd_str: return
        
        if cmd_str == '1': self.send_command(COMMANDS['NO_OP'])
        elif cmd_str == '2': self.send_command(COMMANDS['PAYLOAD_EXEC'], '["take_photo", [], {}]')
        elif cmd_str == '3': self.send_command(COMMANDS['PAYLOAD_TURN_ON'])
        elif cmd_str == '4': self.send_command(COMMANDS['PAYLOAD_TURN_OFF'])
        elif cmd_str == '5': self.send_command(COMMANDS['MANUAL_STATE_OVERRIDE'], "running_state")
        elif cmd_str == '6': self.simulate_packet()
        else: print("Unknown Command.")

    def run(self):
        """Main Loop: Checks Radio and Input continuously."""
        while True:
            # 1. Radio Check (Non-blocking)
            packet = self.radio.receive(timeout=0.05)
            if packet:
                self.process_incoming_packet(packet)
                print(f"\n> {self.input_buffer}", end="")

            # 2. Input Check (Non-blocking)
            if supervisor.runtime.serial_bytes_available:
                char = sys.stdin.read(1)
                if char in ['\n', '\r']:
                    self.handle_user_input(self.input_buffer.strip())
                    self.input_buffer = ""
                    print("\n> ", end="")
                else:
                    self.input_buffer += char
                    print(char, end="")

# ==========================================
# ENTRY POINT
# ==========================================

if __name__ == "__main__":
    try:
        gs = GroundStation()
        gs.run()
    except KeyboardInterrupt:
        print("\nShutting down.")