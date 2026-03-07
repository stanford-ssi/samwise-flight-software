#!/bin/bash
# Build and flash SAMWISE firmware via SWD debug probe
#
# Usage: ./scripts/swd_load.sh [--config=picubed-debug|picubed-flight|picubed-bringup]
# Default config: picubed-debug

set -e

CONFIG="${1:---config=picubed-debug}"

cd "$(git rev-parse --show-toplevel)"

echo "Building :samwise_elf ($CONFIG)..."
bazel build :samwise_elf "$CONFIG"

echo "Flashing via SWD..."
sudo ~/.pico-sdk/openocd/0.12.0+dev/openocd \
    -s ~/.pico-sdk/openocd/0.12.0+dev/scripts \
    -f interface/cmsis-dap.cfg \
    -f target/rp2350.cfg \
    -c "adapter speed 5000" \
    -c "program bazel-bin/samwise.elf verify reset exit"
