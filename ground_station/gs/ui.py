import sys
import time
from . import comms
from . import protocol
from . import config
from . import hardware
from .state import state_manager

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

def get_user_input_with_timeout(prompt, timeout=0.5):
    """Get user input with timeout, checking for packets while waiting"""
    
    print(prompt, end='', flush=True)
    start_time = time.monotonic()
    user_input = ""
    
    while True:
        # Check for packets every 100ms
        if comms.try_get_packet(timeout=0.1):
            # Packet received, redisplay prompt
            print(f"\r{prompt}", end='', flush=True)
        
        # Simple timeout mechanism (CircuitPython doesn't have select)
        if time.monotonic() - start_time > timeout:
            break
            
        time.sleep(0.1)
    
    return None

def configure_authentication():
    """Interactive authentication configuration"""
    print("\n=== Authentication Configuration ===")
    config.config['auth_enabled'] = get_yes_no("Enable HMAC authentication?", True)
    
    if config.config['auth_enabled']:
        new_psk = get_user_input("Enter PSK (leave blank for default)", None)
        if new_psk:
            config.config['packet_hmac_psk'] = new_psk.encode('utf-8')
        
        boot_count_str = get_user_input("Enter boot count", config.config['boot_count'])
        try:
            config.config['boot_count'] = int(boot_count_str)
        except ValueError:
            print("Invalid boot count, using default")
    else:
        print("Authentication disabled - packets will not be authenticated")

def configure_lora_settings():
    """Interactive LoRA configuration"""
    print("\n=== LoRA Configuration ===")
    print(f"Current settings match flight software defaults:")
    print(f"  Frequency: {config.config['frequency']} MHz")
    print(f"  Bandwidth: {config.config['bandwidth']} Hz")
    print(f"  Spreading Factor: {config.config['spreading_factor']}")
    print(f"  Coding Rate: {config.config['coding_rate']}")
    print(f"  CRC: {config.config['crc']}")
    
    if get_yes_no("Modify LoRA settings?", False):
        freq_str = get_user_input("Frequency (MHz)", config.config['frequency'])
        try:
            config.config['frequency'] = float(freq_str)
        except ValueError:
            print("Invalid frequency, using default")
        
        bw_str = get_user_input("Bandwidth (Hz)", config.config['bandwidth'])
        try:
            config.config['bandwidth'] = int(bw_str)
        except ValueError:
            print("Invalid bandwidth, using default")
        
        sf_str = get_user_input("Spreading Factor (6-12)", config.config['spreading_factor'])
        try:
            sf = int(sf_str)
            if 6 <= sf <= 12:
                config.config['spreading_factor'] = sf
            else:
                print("Spreading factor must be 6-12, using default")
        except ValueError:
            print("Invalid spreading factor, using default")
        
        cr_str = get_user_input("Coding Rate (5-8)", config.config['coding_rate'])
        try:
            cr = int(cr_str)
            if 5 <= cr <= 8:
                config.config['coding_rate'] = cr
            else:
                print("Coding rate must be 5-8, using default")
        except ValueError:
            print("Invalid coding rate, using default")
        
        config.config['crc'] = get_yes_no("Enable CRC?", config.config['crc'])

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
            comms.try_get_packet(timeout=0.1)
            
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
                        comms.try_get_packet(timeout=1.0)
                    elif cmd == '1':
                        print("Sending NO_OP...")
                        comms.send_no_op()
                    elif cmd == '2':
                        payload_cmd = get_user_input("Enter payload command", '["take_photo", ["test1"], {"w": 1024, "h": 768, "cell": 128}]')
                        print(f"Sending payload execute: {payload_cmd}")
                        comms.send_payload_exec(payload_cmd)
                    elif cmd == '3':
                        print("Sending payload turn on...")
                        comms.send_payload_turn_on()
                    elif cmd == '4':
                        print("Sending payload turn off...")
                        comms.send_payload_turn_off()
                    elif cmd == '5':
                        state_name = get_user_input("Enter state name", "running_state")
                        print(f"Sending state override: {state_name}")
                        comms.send_manual_state_override(state_name)
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

def debug_listen_mode():
    """Simple debug mode - just listen and report any packets"""
    print("\n=== DEBUG LISTENER MODE ===")
    print("Listening for ANY packets and attempting beacon decode...")
    print("Press Ctrl+C to stop\n")
    
    packet_count = 0
    start_time = time.monotonic()
    
    try:
        while True:
            # Blink LED to show we're alive
            if hardware.led is not None:
                hardware.led.value = True
            
            # Check for any packet with short timeout
            if hardware.rfm9x is not None:
                packet = hardware.rfm9x.receive(timeout=0.1)
            else:
                packet = None
            
            if packet is not None:
                packet_count += 1
                current_time = time.monotonic()
                elapsed = current_time - start_time
                
                print("\n*** PACKET #{} at {:.1f}s ***".format(packet_count, elapsed))
                print("Length: {} bytes".format(len(packet)))
                
                # Convert to hex string
                if hasattr(packet, 'hex'):
                    hex_str = packet.hex()
                else:
                    hex_str = ''.join('{:02x}'.format(b) for b in packet)
                print("Raw hex: {}".format(hex_str))
                
                # Use adafruit_rfm9x library properties to get RadioHead header fields
                try:
                    # Get RadioHead header fields from the library
                    rh_destination = getattr(hardware.rfm9x, 'destination', None)
                    rh_node = getattr(hardware.rfm9x, 'node', None) 
                    rh_identifier = getattr(hardware.rfm9x, 'identifier', None)
                    rh_flags = getattr(hardware.rfm9x, 'flags', None)
                    
                    print("ADAFRUIT RFM9X HEADER FIELDS:")
                    print("  RadioHead TO (destination): {}".format(rh_destination))
                    print("  RadioHead FROM (node): {}".format(rh_node))
                    print("  RadioHead ID (identifier): {}".format(rh_identifier))
                    print("  RadioHead FLAGS: {}".format(rh_flags))
                    
                    # RadioHead stripped dst, src, flags, seq
                    # Beacon packets after RadioHead processing: len(1) + beacon_data(len)
                    # (boot_count, msg_id, hmac are handled at packet layer, not beacon layer)
                    if len(packet) >= 1:
                        # First byte is the beacon data length
                        data_len = packet[0]
                        print("BEACON PACKET STRUCTURE:")
                        print("  beacon_data_len={}".format(data_len))
                        print("  expected_total_size={} bytes".format(1 + data_len))
                        print("  actual_packet_size={} bytes".format(len(packet)))
                        
                        # Extract beacon data payload
                        if len(packet) >= 1 + data_len:
                            beacon_payload = packet[1:1+data_len]
                            
                            # Check RadioHead headers to determine if this is from satellite
                            try:
                                beacon_data = protocol.decode_beacon_data(beacon_payload)
                                print("  >>> BEACON DETECTED (from satellite) <<<")
                                print("  State: {}".format(beacon_data['state_name']))
                                
                                if beacon_data['stats']:
                                    stats = beacon_data['stats']
                                    
                                    # Sync our local state with satellite's boot count
                                    state_manager.update_from_beacon(stats['reboot_counter'])
                                    
                                    print("  Reboots: {}".format(stats['reboot_counter']))
                                    print("  Time in State: {} ms".format(stats['time_in_state_ms']))
                                    print("  RX: {} pkts, {} bytes".format(stats['rx_packets'], stats['rx_bytes']))
                                    print("  TX: {} pkts, {} bytes".format(stats['tx_packets'], stats['tx_bytes']))
                                    print("  Battery: {} mV, {} mA".format(stats['battery_voltage'], stats['battery_current']))
                                    print("  Solar: {} mV, {} mA".format(stats['solar_voltage'], stats['solar_current']))
                                    # Decode device status bits
                                    status = stats['device_status']
                                    status_flags = []
                                    if status & 0x01: status_flags.append("RBF")
                                    if status & 0x02: status_flags.append("solar_charge")
                                    if status & 0x04: status_flags.append("solar_fault")
                                    if status & 0x08: status_flags.append("panel_A")
                                    if status & 0x10: status_flags.append("panel_B")
                                    if status & 0x20: status_flags.append("payload")
                                    print("  Status: 0x{:02x} ({})".format(status, ','.join(status_flags) if status_flags else 'none'))
                                    
                                    # Display ADCS data if present
                                    if beacon_data.get('adcs'):
                                        adcs = beacon_data['adcs']
                                        print("  === ADCS Data ===")
                                        print("  Angular Velocity: {:.6f} rad/s".format(adcs['angular_velocity']))
                                        q = adcs['quaternion']
                                        print("  Quaternion: q0={:.6f}, q1={:.6f}, q2={:.6f}, q3={:.6f}".format(q['q0'], q['q1'], q['q2'], q['q3']))
                                        q_mag = (q['q0']**2 + q['q1']**2 + q['q2']**2 + q['q3']**2)**0.5
                                        print("  Quat Magnitude: {:.6f}".format(q_mag))
                                        print("  ADCS State: {}".format(adcs['state']))
                                        print("  ADCS Boot Count: {}".format(adcs['boot_count']))
                                else:
                                    print("  Beacon decode failed, raw data: {}".format(beacon_payload[:20]))
                            except Exception as e:
                                print("  Error decoding beacon data: {}".format(e))
                            finally:
                                print("  Data: {}".format(beacon_payload))
                        else:
                            print("  Packet too short: need {} bytes, got {}".format(1 + data_len, len(packet)))
                    
                except Exception as e:
                    print("DECODE ERROR: {}".format(e))
                    # Fallback to raw display
                    if len(packet) >= 5:
                        print("Raw first 5 bytes: {}, {}, {}, {}, {}".format(
                            packet[0], packet[1], packet[2], packet[3], packet[4]))
                
                # Show signal strength
                try:
                    rssi = hardware.rfm9x.last_rssi
                    print("RSSI: {} dBm".format(rssi))
                except:
                    pass
                
                print("*** END PACKET ***\n")
            
            if hardware.led is not None: 
                hardware.led.value = False
            time.sleep(0.1)
            
            # Periodic status
            current_time = time.monotonic()
            if current_time - start_time > 30:
                print("[{:.0f}s] Listening... ({} packets so far)".format(current_time - start_time, packet_count))
                start_time = current_time
                
    except KeyboardInterrupt:
        print("\n=== SUMMARY ===")
        print("Total packets: {}".format(packet_count))
        print("Returning to main menu...")
