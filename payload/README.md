# Samwise Payload Software

This repository contains the software for the Raspberry Pi 4B-based imaging payload for the **SSI Samwise 2U Cubesat Mission**. The payload is responsible for managing the camera subsystem and the 2400MHz radio communication.

## Project Overview

The Samwise Payload is designed for a 2U Cubesat. Its primary objectives are:

1. **Multi-Camera Imaging:** Capture images from three different cameras via an Arducam multiplexer.
2. **High-Frequency Downlink:** Transmit telemetry and image data via a 2400MHz radio.
3. **Command & Control:** Process complex imaging and transmission commands sent from the PiCubed flight computer over UART.

## Hardware Requirements

* **Computing:** Raspberry Pi 4B (16GB or 32GB microSD).
* **Imaging:** 3x Raspberry Pi Camera Module V2 (imx219).
* **Multiplexing:** Arducam Quad-camera Multiplexer Board.
* **Radio:** 2400MHz Radio module.
* **Flight Interface:** RPi Interface Board to PiCubed Flight Computer.

---

## Software Setup

### 1. Operating System

Install **Raspberry Pi OS Lite (64-bit)** using Raspberry Pi Imager.

* **Username:** `pi`
* **Password:** `spaghetti`
* **Configuration:** Enable SSH and configure Wi-Fi credentials in the pre-installation settings.

### 2. Repository Configuration

Once logged into the Pi, initialize the workspace:

```bash
# Install Git and GitHub CLI
sudo apt update
sudo apt install git gh -y

# Setup the repository
cd /home/pi/code
git init
gh auth login
git remote add origin https://github.com/QuackWifHat/Samwise-Payload.git

```

### 3. Radio & Support Software

The radio software must be built from source:

```bash
cd radio
mkdir build && cd build
sudo cmake .. && sudo make -j4

```

---

## Communication Protocol

The payload utilizes a layered UART protocol to communicate with the PiCubed flight computer.

### Protocol Layers

1. **Hardware Layer:** Direct serial access via `serial.Serial`.
2. **Packet Handler:** Manages byte streams into validated packets.
3. **Command Handler:** Encodes/decodes JSON strings into executable functions.

### Packet Structure & Synchronization

* **Sync Byte:** The payload waits for the start byte `0x40` (the `@` symbol) to begin processing a header.
* **Header:** Contains 2 bytes for length, 2 bytes for sequence number, and 4 bytes for CRC checksum.
* **Acknowledgement (ACK):** The system uses a "Wait for ACK" cycle. The receiver must send `!` to acknowledge the header before the body is sent, and again after the packet is received.

---

## Command Reference

Commands are sent as JSON-encoded lists: `["command_name", [args], {kwargs}]`.

| Command | Description |
| --- | --- |
| `take_photo` | Captures an image with specified dimensions and cell size. |
| `send_file_2400` | Downlinks a specified file via the 2400MHz radio. |
| `take_process_send_image` | Captures an image, packetizes it with SSDV, and downlinks it via radio. |

*Note: For ground station commands, ensure strings are escaped properly (e.g., `\"command\"`) as the PiCubed/Payload parser requires strict double-quote formatting.*

---

## Testing and Troubleshooting

### Ground Station Interaction

To test the full loop, use the Ground Station to trigger the `PAYLOAD_TURN_ON` and `PAYLOAD_EXEC` commands. If successful, the payload will return `[true, null]`.

### Known Issues

* **UART Hang:** Due to the protocol's safety features, the UART task may "hang" for 1–5 seconds if no commands are being actively processed. Periodic pings can prevent this.
* **Camera Initialization:** There is a known instability with the `imx219` drivers on Debian 13 (Trixie). Use the following diagnostic commands to verify hardware status:
* `rpicam-hello --list-cameras` (List detected sensors)
* `i2cdetect -y 0` (Check the I2C multiplexer)


* **Radio Access:** The radio software requires `sudo` privileges to access the hardware pins.

---
