# 2400 MHz (S-band) Ground Station

Standalone listener for image/file packets transmitted by the payload's S-band
(SX1280) radio. This is a separate system from the 433 MHz primary GS that
lives at the top of `ground_station/`.

This was ported from the `cleanup/2400-gs` branch (commit `953bf31`) into
`main`'s new `ground_station/` layout, with bug fixes from the diagnosis in
`/Users/luis/.claude/plans/valiant-weaving-llama.md`:

- `main.py`: replaced non-blocking `subprocess.Popen` with blocking
  `subprocess.run` (was spawning concurrent `Lora_rx` processes that fought
  for the radio), and fixed the per-iteration filename construction.
- `radio/CMakeLists.txt`: removed the `Test` target — the `Test.cpp` file
  it referenced was never committed and broke `make -j4`.

## Layout

```text
ground_station/2400/
├── main.py            # Python listener wrapper (calls Lora_rx in a loop)
├── setup.py           # GPIO init for the 2400 radio enable line
├── setup.sh           # Apt-installs cmake/git/etc. on a fresh Pi
├── README.md          # (this file)
└── radio/             # C++ SX1280 driver + Lora_tx / Lora_rx executables
    ├── CMakeLists.txt
    ├── Lora_rx.cpp
    ├── Lora_tx.cpp
    ├── SX128x_Linux.cpp
    ├── SX128x_Linux.hpp
    └── README.md
```

The transmitter side is `payload/radio/Lora_tx.cpp`, invoked from
`payload/commands.py:send_file_2400`. Both sides must use matching LoRa
modulation params — currently SF7, BW1600, CR4/8, fixed-length 253-byte
payloads, CRC on, normal IQ — at 2400 MHz.

## Build

On the GS Raspberry Pi:

```bash
cd ground_station/2400/radio
mkdir -p build && cd build
cmake ..
make -j4
```

This produces `Lora_rx` and `Lora_tx` executables in `build/`. The Python
wrapper expects them at `/home/pi/radio/build/Lora_{rx,tx}` — adjust
`RX_2400_EXECUTABLE` / `TX_2400_EXECUTABLE` in `main.py` if you build
elsewhere, or symlink them into place.

## Run

```bash
sudo bash setup.sh    # one-time: install system deps, create /home/pi/{images,videos,logs}
sudo python3 main.py  # start listening
```

`main.py` will block in a loop, calling `Lora_rx` to receive one file at a
time. Received files are written to `/home/pi/images/2400_image_<N>.jpg`
where `<N>` is the count of files already in that directory.

## Hardware notes

Both the payload and GS boards silkscreen `TCXO_EN: 25`, so all four
SX1280 init sites pass GPIO 25 as the 9th element of the pin tuple:

- `payload/radio/Lora_tx.cpp`
- `payload/radio/Lora_rx.cpp`
- `ground_station/2400/radio/Lora_tx.cpp`
- `ground_station/2400/radio/Lora_rx.cpp`

The unused `payload/radio/Lora_tx_packets.cpp` (called by the still-TODO
`send_packets_2400` in `payload/commands.py`) uses a completely different
pin map and is not in sync — fix it before wiring `send_packets_2400` up.

## Known open issues (not fixed in this port)

- **TX power is 0 dBm** in the payload's `Lora_tx.cpp` — marginal at 2.4 GHz.
- **Last-packet edge case:** if the file's size is exactly divisible by 253
  bytes, the TX produces a 0-byte final packet and the RX hangs waiting for
  a partial packet that never arrives. Pad the file or change the sentinel
  protocol to fix.
