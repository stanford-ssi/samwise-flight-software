# Samwise Ground Station

An optimized, mission-ready LoRA communication system designed for coordinating with the Samwise Cubesat. Optimized for high-performance monitoring on Raspberry Pi or CircuitPython microcontrollers.

## 📁 System Architecture

The ground station is designed with a modular, object-oriented architecture to ensure reliable data capture even during high-throughput satellite passes.

```text
ground_station/
├── code.py                    # Main entry point (the firmware/script you run)
├── requirements.txt           # Python dependencies (Pydantic, etc.)
├── gs/                 # Core logic module
│   ├── __init__.py            # Module initialization exposing main APIs
│   ├── config.py              # Radio/protocol settings with flight software code pointers
│   ├── radio_initialization.py # Platform-agnostic hardware setup (Pi/Pico/Feather)
│   ├── radio_commands.py      # OOP LoraRadio class with high-level mission commands
│   ├── protocol.py            # Packet architecture: Base Packet class with inheritance
│   ├── ui.py                  # Non-blocking Interactive UI and Debug Listen modes
│   ├── state.py               # Optimized persistent state manager (low file I/O)
│   ├── models.py              # Pydantic models: Packet, BeaconPacket, AdcsTelemetryPacket
│   └── logger.py              # Mission telemetry logger (CSV + structured console)
├── logs/               # Persistent telemetry CSV archives (Auto-generated)
├── lib/                # CircuitPython libraries (for microcontroller deployment)
└── tests/              # Unit tests for protocol and decoding logic
```

### Key Architectural Improvements:

#### Object-Oriented Radio Interface
The **`LoraRadio`** class in `radio_commands.py` provides a clean OOP wrapper around the low-level RFM9x hardware, encapsulating both hardware access and high-level mission commands.

#### Packet Inheritance Hierarchy 
The **`Packet`** base class in `models.py` handles radio-level protocol (authentication, parsing), while specialized classes like **`BeaconPacket`** and **`AdcsTelemetryPacket`** understand their specific data formats.

#### Optimized State Management
The **`StateManager`** in `state.py` minimizes file I/O for high-throughput scenarios (file transfer protocol requires tens of thousands of packets), using batched writes and atomic file operations.

#### Flight Software Integration
All configuration in **`config.py`** includes code pointers to corresponding flight software implementations, ensuring ground station and satellite stay synchronized.

### Why we split the software this way:
*   **Separation of Concerns**: Decouples radio hardware (`radio_initialization.py`) from orbital protocol (`protocol.py`) and operator interface (`ui.py`).
*   **Mission Reliability**: Heavy operations like telemetry logging and state persistence are optimized to prevent blocking the mission-critical radio polling loop.
*   **Type Safety**: Pydantic models ensure malformed satellite data is caught and handled before reaching operators or databases.
*   **Maintainability**: Code pointers to flight software ensure ground station configurations stay synchronized with satellite implementations.

---

## 🚀 Getting Started

### 1. Main Entrypoint
The **`code.py`** file is the system's entrypoint. It initializes the hardware, loads the latest mission state, and launches the operator menu.

### 2. Running on Raspberry Pi (Recommended)
Perfect for fixed ground stations.
1.  **Install Dependencies**: `pip3 install -r requirements.txt`
2.  **Verify Blinka**: Ensure the Adafruit Blinka library is configured for your Pi's hardware pins.
3.  **Run**: `python3 code.py`

### 3. Running on Microcontrollers (Feather M4 / Pico 2)
1.  Connect via USB (appears as `CIRCUITPY`).
2.  Ensure the `lib/` folder and `gs/` module are copied to the root of the device.
3.  Copy `code.py` to the device root.
4.  Monitor output via a serial terminal (VSCode Serial Monitor or `tio`).

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

Mission-specific settings (Frequency: 438.1 MHz, HMAC Keys, etc.) should be adjusted in **`gs/config.py`**. 

**Note**: The ground station automatically synchronizes its `boot_count` whenever it hears a beacon, ensuring that your next command is always correctly authenticated without manual setup.
