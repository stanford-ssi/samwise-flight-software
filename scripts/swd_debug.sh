#!/bin/bash
# Build, flash, and debug SAMWISE firmware via SWD + GDB
#
# Usage: ./scripts/swd_debug.sh [--config=picubed-debug|picubed-flight|picubed-bringup]
# Default config: picubed-debug
#
# This starts OpenOCD as a GDB server on port 3333, then connects
# arm-none-eabi-gdb. The firmware is flashed before debugging begins.

set -e

CONFIG="${1:---config=picubed-debug}"

cd "$(git rev-parse --show-toplevel)"

echo "Building :samwise_elf ($CONFIG)..."
bazel build :samwise_elf "$CONFIG"

ELF="bazel-bin/samwise.elf"

# Flash first (separate step), then start debug session
echo "Flashing firmware..."
sudo ~/.pico-sdk/openocd/0.12.0+dev/openocd \
    -s ~/.pico-sdk/openocd/0.12.0+dev/scripts \
    -f interface/cmsis-dap.cfg \
    -f target/rp2350.cfg \
    -c "adapter speed 5000" \
    -c "program $ELF verify reset exit"

# Now start OpenOCD as a persistent GDB server
echo "Starting OpenOCD GDB server on :3333..."
sudo ~/.pico-sdk/openocd/0.12.0+dev/openocd \
    -s ~/.pico-sdk/openocd/0.12.0+dev/scripts \
    -f interface/cmsis-dap.cfg \
    -f target/rp2350.cfg \
    -c "adapter speed 5000" &
OPENOCD_PID=$!

sleep 2

trap "sudo kill $OPENOCD_PID 2>/dev/null" EXIT

echo "Connecting GDB..."
arm-none-eabi-gdb "$ELF" \
    -ex "target extended-remote :3333" \
    -ex "monitor reset halt" \
    -ex "break main" \
    -ex "continue"
