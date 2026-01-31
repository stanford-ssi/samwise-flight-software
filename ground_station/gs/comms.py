from . import hardware
from . import protocol
from . import config
from .state import state_manager
from .logger import telemetry_logger, logger
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
            
            logger.info("PACKET RECEIVED | TO: %s | FROM: %s | ID: %s | FLAGS: %s", 
                        rh_destination, rh_node, rh_identifier, rh_flags)
            
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
                        beacon_data.raw_hex = packet.hex() if hasattr(packet, 'hex') else ''.join('{:02x}'.format(b) for b in packet)
                        print(f"  State: {beacon_data.state_name}")
                        
                        if beacon_data.stats:
                            stats = beacon_data.stats
                            print(f"  === Telemetry ===")
                            print(f"Reboots: {stats.reboot_counter}")
                            
                            # Sync our local state with satellite's boot count
                            state_manager.update_from_beacon(stats.reboot_counter)
                            
                            print(f"Time in State: {stats.time_in_state_ms} ms ({stats.time_in_state_ms/1000:.1f} sec)")
                            print(f"RX: {stats.rx_packets} packets, {stats.rx_bytes} bytes")
                            print(f"TX: {stats.tx_packets} packets, {stats.tx_bytes} bytes")
                            
                            print(f"RX Drops - Backpressure: {stats.rx_backpressure_drops}, Bad Packets: {stats.rx_bad_packet_drops}")
                            print(f"Battery: {stats.battery_voltage} mV, {stats.battery_current} mA")
                            print(f"Solar Legacy: {stats.solar_voltage} mV, {stats.solar_current} mA")

                            print(f"Panel A: {stats.panel_A_voltage} mV, {stats.panel_A_current} mA")
                            print(f"Panel B: {stats.panel_B_voltage} mV, {stats.panel_B_current} mA")
                            
                            # Use Pydantic helper for status flags
                            status_flags = stats.device_status_flags
                            print(f"    Device Status: 0x{stats.device_status:02x} ({', '.join(status_flags) if status_flags else 'none'})")
                            
                            # Display ADCS data if present
                            if beacon_data.adcs:
                                adcs = beacon_data.adcs
                                print(f"  === ADCS Telemetry ===")
                                print(f"Angular Velocity: {adcs.angular_velocity:.6f} rad/s")
                                q = adcs.quaternion
                                print(f"Quaternion: q0={q.q0:.6f}, q1={q.q1:.6f}, q2={q.q2:.6f}, q3={q.q3:.6f}")
                                # Use Pydantic helper for magnitude
                                print(f"Quaternion Magnitude: {q.magnitude:.6f} (should be ~1.0)")
                                print(f"ADCS Boot Count: {adcs.boot_count}")
                            
                            if beacon_data.callsign:
                                print(f"Callsign: {beacon_data.callsign}")
                            
                            # Log to persistent storage
                            try:
                                rssi = getattr(hardware.rfm9x, 'last_rssi', None)
                                snr = getattr(hardware.rfm9x, 'last_snr', None)
                                telemetry_logger.log_beacon(beacon_data.dict(), rssi=rssi, snr=snr)
                            except Exception as log_err:
                                print(f"[Error] Failed to log beacon and displays error message: {log_err}")

                        else:
                            logger.warning("Beacon decode failed, raw data: %s", beacon_payload[:20])
                    else:
                        logger.warning("Packet too short: need %d bytes, got %d", 1 + data_len, len(packet))
                else:
                    logger.warning("Packet too short for beacon format")
            else:
                print(f"Type: RESPONSE from node {rh_node}")
                # For command responses, try the old packet format
                try:
                    unpacked = protocol.packet_builder.unpack_packet(packet)
                    # unpacked2 = protocol.packet_builder.unpack_packet(packet2) -> second unpacked string
                    print(f"Data: {unpacked['data']}")
                    if 'boot_count' in unpacked:
                        print(f"Boot Count: {unpacked['boot_count']}")
                        print(f"Message ID: {unpacked['msg_id']}")
                except:
                    print(f"Raw Data: {packet[:20]}")
            
            print(">>> END PACKET <<<\n") # Signals the end of the packet

            return True
        except Exception as e:
            logger.error("ERROR processing packet: %s", e)
            logger.debug("Raw packet data: %s", packet.hex() if hasattr(packet, 'hex') else packet)
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

def send_payload_shutdown():
    """Send payload shutdown command"""
    send_command(config.PAYLOAD_SHUTDOWN)
    
def send_manual_state_override(state_name):
    """Send manual state override command"""
    send_command(config.MANUAL_STATE_OVERRIDE, state_name)
