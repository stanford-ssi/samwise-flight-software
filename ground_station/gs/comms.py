from . import hardware
from . import protocol
from . import config
import time

def try_get_packet(timeout=0.1):
    """Check for incoming packets with short timeout"""
    if hardware.rfm9x is None:
        return False
        
    packet = hardware.rfm9x.receive(timeout=timeout)
    if packet is not None:
        try:
            # Get RadioHead header fields from the library like diagnostics mode
            rh_destination = getattr(hardware.rfm9x, 'destination', None)
            rh_node = getattr(hardware.rfm9x, 'node', None) 
            rh_identifier = getattr(hardware.rfm9x, 'identifier', None)
            rh_flags = getattr(hardware.rfm9x, 'flags', None)
            
            print(f"\n>>> PACKET RECEIVED <<<")
            print(f"  RadioHead TO (destination): {rh_destination}")
            print(f"  RadioHead FROM (node): {rh_node}")
            print(f"  RadioHead ID (identifier): {rh_identifier}")
            print(f"  RadioHead FLAGS: {rh_flags}")
            
            # Check if this is from satellite (beacon source)
            if rh_node == 0:  # FROM satellite
                print(f"  Type: BEACON PACKET")
                
                # RadioHead stripped headers, beacon packets: len(1) + beacon_data(len)
                if len(packet) >= 1:
                    data_len = packet[0]
                    print(f"  beacon_data_len={data_len}")
                    
                    # Extract beacon data payload
                    if len(packet) >= 1 + data_len:
                        beacon_payload = packet[1:1+data_len]
                        beacon_data = protocol.decode_beacon_data(beacon_payload)
                        print(f"  State: {beacon_data['state_name']}")
                        
                        if beacon_data['stats']:
                            stats = beacon_data['stats']
                            print(f"  === Telemetry ===")
                            print(f"    Reboots: {stats['reboot_counter']}")
                            print(f"    Time in State: {stats['time_in_state_ms']} ms ({stats['time_in_state_ms']/1000:.1f} sec)")
                            print(f"    RX: {stats['rx_packets']} packets, {stats['rx_bytes']} bytes")
                            print(f"    TX: {stats['tx_packets']} packets, {stats['tx_bytes']} bytes")
                            print(f"    RX Drops - Backpressure: {stats['rx_backpressure_drops']}, Bad Packets: {stats['rx_bad_packet_drops']}")
                            print(f"    Battery: {stats['battery_voltage']} mV, {stats['battery_current']} mA")
                            print(f"    Solar: {stats['solar_voltage']} mV, {stats['solar_current']} mA")
                            # Decode device status bits
                            status = stats['device_status']
                            status_flags = []
                            if status & 0x01: status_flags.append("RBF_detected")
                            if status & 0x02: status_flags.append("fixed_solar_charge")
                            if status & 0x04: status_flags.append("fixed_solar_fault")
                            if status & 0x08: status_flags.append("panel_A_deployed")
                            if status & 0x10: status_flags.append("panel_B_deployed")
                            if status & 0x20: status_flags.append("payload_on")
                            print(f"    Device Status: 0x{status:02x} ({', '.join(status_flags) if status_flags else 'none'})")
                            
                            # Display ADCS data if present
                            if beacon_data.get('adcs'):
                                adcs = beacon_data['adcs']
                                print(f"  === ADCS Telemetry ===")
                                print(f"    Angular Velocity: {adcs['angular_velocity']:.6f} rad/s")
                                q = adcs['quaternion']
                                print(f"    Quaternion: q0={q['q0']:.6f}, q1={q['q1']:.6f}, q2={q['q2']:.6f}, q3={q['q3']:.6f}")
                                # Calculate quaternion magnitude for validation
                                q_mag = (q['q0']**2 + q['q1']**2 + q['q2']**2 + q['q3']**2)**0.5
                                print(f"    Quaternion Magnitude: {q_mag:.6f} (should be ~1.0)")
                                print(f"    ADCS State: {adcs['state']}")
                                print(f"    ADCS Boot Count: {adcs['boot_count']}")
                        else:
                            print(f"    Beacon decode failed, raw data: {beacon_payload[:20]}")
                    else:
                        print(f"  Packet too short: need {1 + data_len} bytes, got {len(packet)}")
                else:
                    print(f"  Packet too short for beacon format")
            else:
                print(f"  Type: RESPONSE from node {rh_node}")
                # For command responses, try the old packet format
                try:
                    unpacked = protocol.packet_builder.unpack_packet(packet)
                    print(f"  Data: {unpacked['data']}")
                    if 'boot_count' in unpacked:
                        print(f"  Boot Count: {unpacked['boot_count']}")
                        print(f"  Message ID: {unpacked['msg_id']}")
                except:
                    print(f"  Raw Data: {packet[:20]}")
            
            print(">>> END PACKET <<<\n")
            return True
        except Exception as e:
            print(f"\n>>> ERROR processing packet: {e} <<<")
            print(f"Raw packet data: {packet.hex() if hasattr(packet, 'hex') else packet}")
            return False
    return False

def send_command(cmd_id, cmd_payload="", dst=0xFF):
    """Send a command packet to the flight software"""
    if hardware.rfm9x is None:
        print("Error: Radio not initialized")
        return

    data = protocol.create_cmd_payload(cmd_id, cmd_payload)
    
    # Create our packet payload (without RadioHead headers)
    # Flight software expects: dst, src, flags, seq, len, data, boot_count, msg_id, hmac
    packet_payload = protocol.packet_builder.create_packet(
        dst=dst,  # Our destination address
        src=0xFF,  # Our source address (ground station)
        flags=0x00,  # No special flags
        seq=0x00,  # Sequence number
        data=data,  # Command data
    )
    
    # Do NOT strip the header. The flight software expects the full packet structure 
    # (dst, src, flags, seq) as the payload, even if RadioHead also has headers.
    payload_to_send = packet_payload
    
    print("Packet dump: " + " ".join("{:02x}".format(byte) for byte in payload_to_send))
    
    # Use adafruit_rfm9x library's RadioHead headers
    hardware.rfm9x.send(
        payload_to_send,
        destination=0xFF,    # RadioHead TO field
        node=0x00,          # RadioHead FROM field (ground station)
        identifier=0x00,    # RadioHead ID field 
        flags=0x00          # RadioHead FLAGS field
    )
    
    print("Sent packet with RadioHead headers: TO={}, FROM={}, ID={}, FLAGS={}".format(
        dst, 0xFF, 0x00, 0x00))
    print("Payload length: {} bytes".format(len(payload_to_send)))

def send_no_op():
    """Send a NO_OP ping command"""
    send_command(config.NO_OP)
    
def send_payload_exec(command_str):
    """Send a payload execution command"""
    send_command(config.PAYLOAD_EXEC, command_str)
    
def send_payload_turn_on():
    """Send payload turn on command"""
    send_command(config.PAYLOAD_TURN_ON)
    
def send_payload_turn_off():
    """Send payload turn off command"""
    send_command(config.PAYLOAD_TURN_OFF)
    
def send_manual_state_override(state_name):
    """Send manual state override command"""
    send_command(config.MANUAL_STATE_OVERRIDE, state_name)
