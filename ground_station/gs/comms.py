from . import hardware
from . import protocol
from . import config
from .state import state_manager
from .logger import telemetry_logger
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
                        beacon_data['raw_hex'] = packet.hex() if hasattr(packet, 'hex') else ''.join('{:02x}'.format(b) for b in packet)
                        print(f"  State: {beacon_data['state_name']}")
                        
                        if beacon_data['stats']:
                            stats = beacon_data['stats']
                            print(f"  === Telemetry ===")
                            print(f"    Reboots: {stats['reboot_counter']}")
                            
                            # Sync our local state with satellite's boot count
                            state_manager.update_from_beacon(stats['reboot_counter'])
                            
                            print(f"    Time in State: {stats['time_in_state_ms']} ms ({stats['time_in_state_ms']/1000:.1f} sec)")
                            print(f"    RX: {stats['rx_packets']} packets, {stats['rx_bytes']} bytes")
                            print(f"    TX: {stats['tx_packets']} packets, {stats['tx_bytes']} bytes")
                            print(f"    RX Drops - Backpressure: {stats['rx_backpressure_drops']}, Bad Packets: {stats['rx_bad_packet_drops']}")
                            print(f"    Battery: {stats['battery_voltage']} mV, {stats['battery_current']} mA")
                            print(f"    Solar Legacy: {stats['solar_voltage']} mV, {stats['solar_current']} mA")
                            print(f"    Panel A: {stats['panel_A_voltage']} mV, {stats['panel_A_current']} mA")
                            print(f"    Panel B: {stats['panel_B_voltage']} mV, {stats['panel_B_current']} mA")
                            
                            # Decode device status bits
                            status = stats['device_status']
                            status_flags = []
                            if status & 0x01: status_flags.append("RBF_detected")
                            if status & 0x02: status_flags.append("solar_charge")
                            if status & 0x04: status_flags.append("solar_fault")
                            if status & 0x08: status_flags.append("panel_A_deployed")
                            if status & 0x10: status_flags.append("panel_B_deployed")
                            if status & 0x20: status_flags.append("payload_on")
                            if status & 0x40: status_flags.append("adcs_on")
                            if status & 0x80: status_flags.append("adcs_valid")
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
                                print(f"    ADCS Boot Count: {adcs['boot_count']}")
                            
                            if beacon_data.get('callsign'):
                                print(f"  Callsign: {beacon_data['callsign']}")
                            
                            # Log to persistent storage
                            try:
                                rssi = getattr(hardware.rfm9x, 'last_rssi', None)
                                snr = getattr(hardware.rfm9x, 'last_snr', None)
                                telemetry_logger.log_beacon(beacon_data, rssi=rssi, snr=snr)
                            except Exception as log_err:
                                print(f"  [Error] Failed to log beacon: {log_err}")

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
    
    # Create our packet payload
    packet_payload = protocol.packet_builder.create_packet(
        dst=dst,
        src=0xFF,
        flags=0x00,
        seq=0x00,
        data=data,
    )
    
    # Send using library
    hardware.rfm9x.send(
        packet_payload,
        destination=0xFF,
        node=0x00,
        identifier=0x00,
        flags=0x00
    )
    
    print(f"Sent command {cmd_id} (Payload: {cmd_payload})")

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
