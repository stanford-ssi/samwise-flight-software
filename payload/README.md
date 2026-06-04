# Samwise Payload Software

This repository contains the software for the Raspberry Pi 4B-based imaging payload for the **SSI Samwise 2U Cubesat Mission**. The payload is responsible for managing the camera subsystem and the 2400MHz radio communication.

Setup link: [HERE](https://docs.google.com/document/d/199YqVAPGcxWh0DG2zB8p-KAbreFfaThLlTS_5SFfKP8/edit?tab=t.0)

## Project Overview

The Samwise Payload is designed for a 2U Cubesat. Its primary objectives are:

1. **Multi-Camera Imaging:** Capture images from three different cameras via an Arducam multiplexer.
2. **High-Frequency Downlink:** Transmit telemetry and image data via a 2400MHz radio.
3. **Command & Control:** Process complex imaging and transmission commands sent from the PiCubed flight computer over UART.

---

## Software Setup

### 1. Operating System

**PREREQUISITE:** Installed **Raspberry Pi OS Lite (64-bit)** using [Raspberry Pi Imager](https://www.raspberrypi.com/software/).

> [!WARNING]
> Ensure your installation meets the specifications on the arducam website:
>
> *We currently only guarantee that the Camera Adaptation is supported on Raspberry Pi Bullseye and **Bookworm OS**.*
> *Please make sure your kernel version is up to date (at least 5.15.63 or later)" with `uname -r`*

**Configuration:** Enable SSH and configure Wi-Fi credentials in the pre-installation settings.

If you are new to the command line, you can find some helpful commands [HERE](https://raspberrypi-guide.github.io/programming/working-with-the-command-line).
If there are any complications using `sudo raspi-config` for Wi-Fi setup on an installed RPi, connect using nmcli:

```bash
nmcli dev wifi rescan
nmcli dev wifi list
sudo nmcli dev wifi connect AA:BB:CC:DD:EE:FF password "{PASSWORD}" ifname wlan0
```

### 2. Repository Configuration

Note: From this point forward, these instructions assume you’re running commands as the 'pi' user (adjust as needed).

Once logged into the RPi, run `mkdir /home/pi/code`. Copy the contents of the payload folder into the /home/pi/code by initializing the workspace:

```bash
# Install Git and GitHub CLI
sudo apt update
sudo apt install git gh -y
# Verify installation
git --version
gh --version

# Setup the repository
git clone https://github.com/stanford-ssi-samwise-flight-software.git
cp -a /home/pi/samwise-flight-software/payload/. /home/pi/code/
# OPTIONAL: delete repo rm -r /home/pi/samwise-flight-software
cd /home/pi/code
git clone

# Run the following commands
cd /home/pi/code
sudo bash setup.sh
```

### 3. Radio & Support Software

The radio software must be built from source:

```bash
cd /home/pi/code/radio
mkdir build && cd build
sudo cmake .. && sudo make -j4
```
---
## HardWare Setup

Follow the detailed instructions located [HERE](https://docs.google.com/document/d/199YqVAPGcxWh0DG2zB8p-KAbreFfaThLlTS_5SFfKP8/edit?tab=t.0). The final setup should look like [THIS].<img width="512" height="288" alt="image" src="https://github.com/user-attachments/assets/6a9fce41-a218-420d-a237-5cb896772f5c" />
<img width="512" height="288" alt="image" src="https://github.com/user-attachments/assets/13d3a4c8-5269-4e63-8c81-ad433290003b" />

### Hardware Requirements

* **Computing:** Raspberry Pi 4B with software loaded.
* **Flight Interface:** RPi Interface Board.
* **Multiplexing:** Arducam Camera Multiplexer Board.
* **Imaging:** 2x RPi Camera V2.
* **Cabling:** 2x Medium-length RPi Camera V2 ribbon connectors; 1x Short-length RPi Camera V2 ribbon connector.
* **Radio:** Radio antenna for the RPi Interface Board.
* **Harness:** RPi Interface to PiCubed harness.

---

## RPi Camera Initialization & Testing

This section is intended to help verify that all Raspberry Pi Camera Module V2 sensors (imx219) are detected and usable through the Arducam quad multiplexer. Follow the setup document (setup works without antenna) linked above and ensure ribbon cables are set correctly.

### Expectations

There is a known instability with the `imx219` drivers on Debian 13 (Trixie). Use the following diagnostic commands to verify hardware status:
* `rpicam-hello --list-cameras` (List detected sensors)
* `rpicam-still --camera <num>` (captures image for specified camera)
* `i2cdetect -y 0` (Check the I2C multiplexer)

### Mux Overlay Configuration

If the cameras enumerate inconsistently or captures time out, ensure that auto-detect is disabled and the mux overlay is explicitly enabled.
Update fully:
```bash
sudo apt update && sudo apt full-upgrade -y
sudo reboot

```
Edit firmware config:
```bash
sudo nano /boot/firmware/config.txt

```
Find and update the following in `/boot/firmware/config.txt`:

```bash
camera_auto_detect=1
```

Change it to:
```bash
camera_auto_detect=0
```

Under the [all] section at the bottom, add your number of cameras:
```bash
dtparam=i2c_vc=on
dtoverlay=camera-mux-4port,cam0-imx219,cam2-imx219,cam3-imx219
```
Then reboot and rerun the diagnostic commands.

Notes: cam0 maps to camera A and cam2 maps to camera C (based on current lab wiring). There has been variance among RPIs in which `cam#` combinations work reliably; if needed, test alternate combinations and document what works for that specific Pi. If using other connection points, enable the matching cams:

* **Camera A** → cam0
* **Camera B** → cam1
* **Camera C** → cam2
* **Camera D** → cam3

Reference: [Arducam Multi-Camera Adapter Quick Start Guide](https://docs.arducam.com/Raspberry-Pi-Camera/Multi-Camera-CamArray/Quick-Start-Guide-for-Multi-Adapter-Board/)

---

## RPi Camera Command Reference

Commands can also be run locally from the payload directory using the provided Python CLIs:

| Script / Command | Description |
| --- | --- |
| `python take_photo_cli.py` | Interactive photo capture. Prompts for `image_id` and (optionally) camera/config/size/quality/cell settings, then calls `commands.take_photo(...)`. |
| `python take_photo_cli.py IMAGE_ID [--cam-num N] [--camera-name A] [--config default] [-w WIDTH] [-H HEIGHT] [-q QUALITY] [--cells-x X] [--cells-y Y] [--ssdv]` | Non-interactive photo capture with CLI args. If `--ssdv` is set, the output JPEG is SSDV-encoded after capture. |
| `python take_vid_cli.py` | Interactive video capture. Prompts for `vid_id` and (optionally) camera/config profiles, then calls `commands.take_vid(...)`. |
| `python take_vid_cli.py VIDEO_ID [--camera-num N] [--camera-name A] [--libcamera-config default] [--ffmpeg-in-config default] [--ffmpeg-out-config default]` | Non-interactive video capture with CLI args. Produces a raw + compressed video (size stats printed after completion). |

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

## Ground Station Command Reference

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

* **Radio Access:** The radio software requires `sudo` privileges to access the hardware pins.

---
