#!/bin/bash
# Build, flash, and debug SAMWISE firmware via SWD on WSL2
#
# Usage: ./scripts/swd_wsl.sh [--config=picubed-debug|picubed-flight|picubed-bringup]
# Default config: picubed-debug
#
# Flashing and the OpenOCD GDB server run on Windows (requires Admin).
# A UAC prompt will appear automatically.

set -e

CONFIG="${1:---config=picubed-debug}"
WIN_IP=$(ip route show | grep default | awk '{print $3}')
WIN_TEMP="/mnt/c/Users/ayush/AppData/Local/Temp"

cd "$(git rev-parse --show-toplevel)"

echo "Building :samwise_elf ($CONFIG)..."
bazel build :samwise_elf "$CONFIG"

echo "Flashing via picotool..."
sudo picotool load bazel-bin/samwise.elf -f
sudo picotool reboot -f

# Write the GDB server script to Windows temp so it can be run elevated
cat > "$WIN_TEMP/swd_server.ps1" << 'EOF'
$OPENOCD = "C:\Users\ayush\.pico-sdk\openocd\0.12.0+dev\openocd.exe"
$SCRIPTS  = "C:\Users\ayush\.pico-sdk\openocd\0.12.0+dev\scripts"

Write-Host "Starting OpenOCD GDB server on :3333..."
& $OPENOCD -s $SCRIPTS -f interface/cmsis-dap.cfg -f target/rp2350.cfg `
    -c "bindto 0.0.0.0" `
    -c "adapter speed 5000" `
    -c "gdb_memory_map disable" `
    -c "gdb_flash_program disable"
EOF

echo ""
echo "==> Open an Admin PowerShell on Windows and run:"
echo "    powershell -NoExit -File C:\Users\ayush\AppData\Local\Temp\swd_server.ps1"
echo ""
read -n 1 -s -p "Press any key once the GDB server is listening on :3333..."
echo

echo "Connecting GDB..."
arm-none-eabi-gdb bazel-bin/samwise.elf \
    -ex "target extended-remote $WIN_IP:3333" \
    -ex "monitor reset halt" \
    -ex "break main" \
    -ex "continue"
