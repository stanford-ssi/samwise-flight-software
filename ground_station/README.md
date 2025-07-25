# Ground Station

Interactive LoRA communication system for communicating with the Samwise satellite.

## Hardware Setup

### Supported Boards
- Raspberry Pi Pico 2
- Adafruit Feather M4

### Required Hardware
- Compatible microcontroller board (Pico 2 or Feather M4)
- RFM9x LoRA radio module
- Proper antenna for 438.1 MHz
- USB connection for monitoring output

## Software Setup

### 1. Install CircuitPython

Install CircuitPython on your microcontroller:
- **Pico 2**: Follow instructions at https://circuitpython.org/board/raspberry_pi_pico2/
- **Feather M4**: Follow instructions at https://circuitpython.org/board/feather_m4_express/

### 2. Install Required Libraries

Copy the required libraries to your CircuitPython device:

1. Connect your microcontroller via USB (it should appear as a drive named `CIRCUITPY`)
2. Copy all files from `ground_station/lib/` to the `lib/` folder on your device
3. The main required library is `adafruit_rfm9x.mpy` (from https://github.com/adafruit/Adafruit_CircuitPython_RFM9x)

### 3. Deploy Ground Station Code

Copy `ground_station/code.py` to the root of your CircuitPython device as `code.py`

## Running the Ground Station

### Using `tio`

1. **Install cli tool**: (MacOS) `brew install tio`
2. Look for the device plugged in:
   - **Windows**: Usually `COM3`, `COM4`, etc.
   - **Mac**: Usually `/dev/cu.usbmodem*` or `/dev/tty.usbmodem*`
   - **Linux**: Usually `/dev/ttyACM0` or `/dev/ttyUSB0`
3. Connect to it using:
   - `tio /dev/tty.usbmodem1101 -t`: Make sure you change the name of the device appropriately

**Note: If it's not responding, you don't see anything printing, use CTRL+C to interrupt it and follow on-screen instructions**

### Setting Up Serial Monitor in VSCode

1. **Install Extension**: Install Microsoft's "Serial Monitor" extension in VSCode
2. **Connect Hardware**: Connect your microcontroller via USB
3. **Open Serial Monitor**: 
   - Press `Ctrl+Shift+P` (or `Cmd+Shift+P` on Mac)
   - Type "Serial Monitor: Start Monitoring"
   - Or find "Serial Monitor" in the bottom panel/terminal area
4. **Select Port**: Choose the correct serial port:
   - **Windows**: Usually `COM3`, `COM4`, etc.
   - **Mac**: Usually `/dev/cu.usbmodem*` or `/dev/tty.usbmodem*`
   - **Linux**: Usually `/dev/ttyACM0` or `/dev/ttyUSB0`
5. **Set Baud Rate**: Use `115200` (default for CircuitPython)
6. **Start Listening**: Click "Start Monitoring" or press the play button

### Basic Execution

1. With serial monitor connected, reset the microcontroller or press `Ctrl+D`
2. The ground station will start automatically and display the mode selection menu

### Operation Modes

The ground station offers two modes:

#### 1. Debug Listen Mode
Continuously monitors for incoming packets and provides detailed analysis.

Features:
- Real-time packet monitoring
- Detailed packet structure analysis
- Automatic beacon packet detection and decoding
- Signal strength reporting (RSSI)
- Telemetry statistics display

#### 2. Interactive Command Mode (**DO NOT USE**)
Allows sending commands to the satellite and monitoring responses.

Available commands:
- NO_OP (ping)
- Payload Execute
- Payload Turn On/Off
- Manual State Override

WARNING: This mode has not yet been fully debugged and is missing key features necessary to send a valid command instruction or receive beacons from the satellite!

### Debug Mode Usage

When running in debug mode, the system displays detailed packet information:

```
*** PACKET #1 at 5.2s ***
Length: 45 bytes
Raw hex: 01054c6f72656d...
ADAFRUIT RFM9X HEADER FIELDS:
  RadioHead TO (destination): 255
  RadioHead FROM (node): 0
  RadioHead ID (identifier): 0
  RadioHead FLAGS: 0
BEACON PACKET STRUCTURE:
  beacon_data_len=20
  expected_total_size=21 bytes
  actual_packet_size=45 bytes
  >>> BEACON DETECTED (from satellite) <<<
  State: running_state
  Reboots: 1
  Time in State: 15000 ms
  RX: 5 pkts, 225 bytes
  TX: 3 pkts, 135 bytes
RSSI: -85 dBm
*** END PACKET ***
```

### Beacon Packet Analysis

Debug mode automatically detects beacon packets (source node = 0) and decodes:
- **State name** - Current satellite state
- **Telemetry statistics**:
  - Reboot counter
  - Time in current state (ms)
  - RX/TX packet and byte counts
  - Packet drop statistics

## Configuration

The system uses default settings that match the flight software:
- **Frequency**: 438.1 MHz
- **Bandwidth**: 125000 Hz
- **Spreading Factor**: 7
- **Coding Rate**: 5
- **CRC**: Enabled
- **Authentication**: HMAC enabled with default PSK

## Troubleshooting

If no packets are received:
1. Verify antenna connection
2. Check frequency settings match satellite
3. Ensure satellite is transmitting
4. Check signal strength (RSSI values)
5. Verify board type detection is correct

Press `Ctrl+C` to stop debug mode or return to main menu.

