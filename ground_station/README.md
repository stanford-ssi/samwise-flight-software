# Samwise Ground Station

An optimized, mission-ready LoRa communication system designed for coordinating with the Samwise CubeSat. Targets Raspberry Pi (CPython) for permanent stations and CircuitPython microcontrollers (Pico 2 / Feather M4) for portable use.

## 📁 System Architecture

The ground station is designed with a modular, object-oriented architecture to ensure reliable data capture even during high-throughput satellite passes.

```text
ground_station/                  # Project root (pyproject.toml, uv.lock, README, tests)
├── pyproject.toml
├── uv.lock
├── tests/                       # pytest suite (mocks hardware via conftest.py)
├── lib/                         # CircuitPython .mpy libraries (deployed to device)
└── ground_station/              # The package itself
    ├── __init__.py
    ├── __main__.py              # `python -m ground_station` enters here
    ├── main.py                  # Dispatcher: picks server / code mode
    ├── server.py                # FastAPI server + WebSocket dashboard (primary mode)
    ├── code.py                  # Interactive CLI loop; also CircuitPython boot file
    ├── config.py                # Radio/protocol settings; pointers to flight SW
    ├── radio_initialization.py  # Hardware setup (Pi / Pico / Feather)
    ├── radio_commands.py        # LoraRadio class (low-level + mission commands)
    ├── protocol.py              # Packet encoding/decoding, HMAC auth
    ├── ui.py                    # Non-blocking interactive UI + debug listen
    ├── state.py                 # Persistent state manager (batched I/O)
    ├── logger.py                # Structured console + telemetry CSV logger
    ├── static/                  # Dashboard HTML/JS served by FastAPI
    └── models/                  # Data models (Pydantic on CPython, plain classes on CircuitPython)
        ├── __init__.py
        ├── adcs_model.py
        ├── beacon_model.py
        ├── packet_model.py
        └── command_model.py
```

### Architectural notes

- **Single entry, multiple modes.** `python -m ground_station` runs `main.py`, which dispatches to `server.run()` by default. Override at runtime with `python -m ground_station code` for the interactive loop, or change `DEFAULT_MODE` in `ground_station/main.py`.
- **Object-oriented radio.** `LoraRadio` in `radio_commands.py` wraps the RFM9x with mission-level commands.
- **State manager.** `state.py` batches writes to avoid stalling the radio polling loop during high-throughput passes (file transfer can be tens of thousands of packets).
- **Cross-platform models.** `models/__init__.py` detects CPython vs CircuitPython and switches between Pydantic and a minimal `_BaseModel`.
- **Flight-software pointers.** `config.py` references the corresponding flight-software files for every shared constant so the two sides stay in sync.

### Why we split the software this way:
*   **Separation of Concerns**: Decouples radio hardware (`radio_initialization.py`) from orbital protocol (`protocol.py`) and operator interface (`ui.py`).
*   **Mission Reliability**: Heavy operations like telemetry logging and state persistence are optimized to prevent blocking the mission-critical radio polling loop.
*   **Type Safety**: Data models provide structured validation (Pydantic on RPi, simple classes on CircuitPython) to ensure malformed satellite data is caught and handled.
*   **Platform Compatibility**: Automatic detection and graceful degradation between CPython (RPi) and CircuitPython (Pico/Feather).
*   **Maintainability**: Code pointers to flight software ensure ground station configurations stay synchronized with satellite implementations.

---

## 🚀 Getting Started (Raspberry Pi / CPython)

The project uses [uv](https://github.com/astral-sh/uv) for environment + dependency management.

### Install

```bash
cd ground_station            # the project root (where pyproject.toml lives)
uv sync                      # installs all deps into .venv
```

`requires-python` is `>=3.13`.

### Run

```bash
# Default: FastAPI server on http://127.0.0.1:8000
uv run python -m ground_station

# Interactive Code mode (command line), including debug listen mode
uv run python -m ground_station code
```

The dashboard is at <http://localhost:8000>.

### Tests

```bash
uv run pytest
```

Hardware modules (`board`, `busio`, `digitalio`, `adafruit_rfm9x`) are mocked in `tests/conftest.py`, so the suite runs anywhere.

---

**TROUBLESHOOTING STEP**: If you get an error like 'GPIO Busy':

1. Run `sudo raspi-config` and enable "SPI" (`Interface` -> `SPI` -> `Yes` (enable))
2. Change `/boot/firmware/config.txt` with the following:
    * Make sure `dtparam=spi=on`
    * Add AT THE END (underneath `[all]`): `dtoverlay=spi0-1cs`.
3. `sudo reboot` and it should work.

### 3. Running on CircuitPython Microcontrollers (Pico 2 / Feather M4)
Perfect for portable/field ground stations.

#### Hardware Requirements
- **Pico 2** or **Feather M4 Express** board
- **RFM9x LoRA Radio Module** (433/868/915 MHz)
- **Proper antenna** tuned for your frequency (438.1 MHz for Samwise)
- **USB cable** for power and serial monitoring

#### Step-by-Step Deployment

**1. Install CircuitPython Firmware**
- **Pico 2**: Download from [circuitpython.org/board/raspberry_pi_pico2](https://circuitpython.org/board/raspberry_pi_pico2/)
- **Feather M4**: Download from [circuitpython.org/board/feather_m4_express](https://circuitpython.org/board/feather_m4_express/)
- Hold BOOTSEL button while plugging in USB
- Drag the `.uf2` file to the `RPI-RP2` drive
- Board will reboot and appear as `CIRCUITPY`

**2. Install Required Libraries**
The ground station needs these CircuitPython libraries (already included in `lib/`):
```bash
# Copy the entire lib/ folder to your device
cp -r lib/ /Volumes/CIRCUITPY/
```

Libraries included:
- `adafruit_rfm9x.mpy` - LoRA radio driver
- `adafruit_hashlib/` - SHA256 for HMAC authentication
- `circuitpython_hmac.mpy` - HMAC for packet authentication

**3. Deploy Ground Station Code**
Copy all Python files to the root of the `CIRCUITPY` drive:
```bash
# From the ground_station/ directory:
cd /path/to/samwise-flight-software/ground_station

# Copy all Python modules
cp *.py /Volumes/CIRCUITPY/

# The device should now have:
# /CIRCUITPY/code.py
# /CIRCUITPY/__init__.py
# /CIRCUITPY/config.py
# /CIRCUITPY/models.py
# /CIRCUITPY/protocol.py
# /CIRCUITPY/radio_initialization.py
# /CIRCUITPY/radio_commands.py
# /CIRCUITPY/state.py
# /CIRCUITPY/ui.py
# /CIRCUITPY/logger.py
# /CIRCUITPY/lib/ (with all .mpy files)
```

**4. Connect and Monitor**

Using **tio** (recommended):
```bash
# macOS/Linux
brew install tio  # or apt install tio

# Find your device (usually /dev/tty.usbmodem* on Mac)
ls /dev/tty.*

# Connect
tio /dev/tty.usbmodem1101

# Press Ctrl+C to interrupt, Ctrl+D to reload
```

Using **VSCode Serial Monitor**:
1. Install "Serial Monitor" extension
2. Press `Ctrl+Shift+P` → "Serial Monitor: Start Monitoring"
3. Select port (e.g., `/dev/tty.usbmodem*` or `COM3`)
4. Set baud rate: `115200`

**5. Verify Operation**
You should see:
```
=== Samwise Ground Station ===
Interactive LoRA Communication System (Refactored)

Select mode:
1. Debug Listen Mode (watch for any packets)
2. Interactive Command Mode

Enter choice (1 or 2):
```

#### CircuitPython Limitations
⚠️ **Known Differences from Raspberry Pi:**
- **No Pydantic**: Models use simple Python classes instead (automatic fallback)
- **Limited CSV logging**: Works but may be slower for high-throughput scenarios
- **Degraded interactive mode**: Input checking uses polling instead of `select()`
- **Memory constraints**: Less RAM available than RPi (2MB on Pico vs 1GB+ on RPi)

✅ **What Works on CircuitPython:**
- Radio communication (RX/TX)
- Beacon decoding
- Debug Listen Mode
- HMAC authentication
- State persistence
- Telemetry logging

#### Troubleshooting CircuitPython

**Issue: Code not running**
- Press `Ctrl+C` then `Ctrl+D` in serial terminal to soft reset
- Check for syntax errors in serial output
- Verify all files copied correctly

**Issue: Import errors**
```python
ImportError: no module named 'adafruit_rfm9x'
```
- Ensure `lib/` folder is copied to device root
- Check that `.mpy` files match your CircuitPython version

**Issue: Radio not initializing**
```
OSError: [Errno 19] No such device
```
- Verify SPI wiring (check `radio_initialization.py` for pin mappings)
- Ensure RFM9x module is properly connected
- Try different CS/RESET pins if needed

**Issue: Out of memory**
```
MemoryError: memory allocation failed
```
- CircuitPython has limited RAM (~2MB)
- Reduce logging verbosity in `logger.py`
- Consider using RPi for high-throughput operations

---

## 🛠️ Operations & Modes

### 📡 Debug Listen Mode
A "read-only" high-speed monitoring mode.
*   **Continuous radio polling**: Optimized for 10ms latency.
*   **Automated Beacon Decoding**: Instantly parses incoming beacons into human-readable telemetry.
*   **UTC Centric**: All timestamps follow ISO-8601 UTC for coordination with mission TLEs.

### 🎮 Interactive Command Mode
The primary mode for satellite control.
*   **Non-Blocking Logic**: The radio continues to monitor for satellite packets *even while you are typing commands*. You will never miss data while interacting with the menu.
*   **HMAC Authentication**: Every outgoing command is cryptographically signed and sequence-checked to prevent replays.
*   **Lazy Saving**: State is cached in RAM and committed to the SD card periodically to protect your hardware during high-speed burst operations.

---

## 📊 Mission Telemetry

Every packet received is automatically archived in **`logs/telemetry_log.csv`**.
*   **Raw Traceability**: The logs include the raw hex payload for forensic recovery.
*   **Link Quality**: RSSI and SNR (Signal-to-Noise Ratio) are tracked for every packet to analyze antenna performance during a pass.

---

## ⚙️ Configuration

Mission-specific settings (Frequency: 438.1 MHz, HMAC Keys, etc.) should be adjusted in **`config.py`**.

**Note**: The ground station automatically synchronizes its `boot_count` whenever it hears a beacon, ensuring that your next command is always correctly authenticated without manual setup.

### 🛡️ Packet Filtering (Noise Rejection)

The ground station includes built-in filters to reject noisy packets not from the Samwise satellite:

#### **1. RSSI Threshold Filter**
Drops packets with signal strength below -120 dBm (very weak/noisy signals).

```python
# In config.py
RSSI_THRESHOLD = -120          # Minimum signal strength in dBm
ENABLE_RSSI_FILTER = True      # Enable/disable RSSI filtering
```

**When a packet is dropped:**
```
PACKET DROPPED | RSSI too low: -125 dBm < -120 dBm threshold
```

#### **2. Callsign Verification**
Verifies that beacons contain the expected amateur radio callsign suffix (**KC3WNY**) assigned to Samwise.

```python
# In config.py
EXPECTED_CALLSIGN = "KC3WNY"   # FCC-assigned callsign for Samwise
ENABLE_CALLSIGN_FILTER = True  # Enable/disable callsign verification
```

**When a packet is dropped:**
```
PACKET DROPPED | Callsign mismatch: 'AB1CDE' (expected 'KC3WNY')
```

**Use Cases:**
- **Dense RF environments**: Filter out packets from other LoRA devices
- **Satellite passes**: Reject ground reflections and multipath interference
- **Testing/debugging**: Disable filters to see all received packets

**To disable filtering** (e.g., for debugging):
```python
config = {
    # ... other settings ...
    'enable_rssi_filter': False,      # Accept all signal strengths
    'enable_callsign_filter': False,  # Accept all callsigns
}
```

---

## 🔧 Platform Comparison

| Feature | Raspberry Pi (CPython) | CircuitPython (Pico/Feather) |
|---------|------------------------|------------------------------|
| **Radio Communication** | ✅ Full support | ✅ Full support |
| **Debug Listen Mode** | ✅ Full support | ✅ Full support |
| **Interactive Command Mode** | ✅ Full support | ⚠️ Degraded (polling-based input) |
| **Beacon Decoding** | ✅ Pydantic validation | ✅ Simple classes |
| **HMAC Authentication** | ✅ Full support | ✅ Full support |
| **CSV Telemetry Logging** | ✅ High-throughput | ⚠️ Limited by RAM/storage |
| **State Persistence** | ✅ Atomic file ops | ⚠️ Basic file ops |
| **Memory Available** | 1GB+ | ~2MB |
| **Best Use Case** | **Permanent ground stations** | **Portable/field operations** |

**Recommendation**: Use **Raspberry Pi** for primary ground stations. Use **CircuitPython** devices for portable/backup stations.
