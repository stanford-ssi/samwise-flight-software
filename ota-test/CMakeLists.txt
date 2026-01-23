cmake_minimum_required(VERSION 3.13)

# Include the Pico SDK
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(rp2350_bootloader_system C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the Pico SDK
pico_sdk_init()

# ============================================================================
# BOOTLOADER
# ============================================================================

add_executable(bootloader
    bootloader.c
)

# Link libraries
target_link_libraries(bootloader
    pico_stdlib
    hardware_watchdog
    hardware_reboot
    hardware_sync
)

# Use custom linker script for bootloader
pico_set_linker_script(bootloader ${CMAKE_CURRENT_LIST_DIR}/bootloader.ld)

# Enable USB output, disable UART
pico_enable_stdio_usb(bootloader 1)
pico_enable_stdio_uart(bootloader 0)

# Create map file
pico_add_extra_outputs(bootloader)

# ============================================================================
# PARTITION A - Stable Baseline
# ============================================================================

add_executable(partition_a
    partition_a.c
)

# Link libraries
target_link_libraries(partition_a
    pico_stdlib
    hardware_flash
    hardware_sync
    hardware_watchdog
    pico_bootrom
)

# Use custom linker script for partition A
pico_set_linker_script(partition_a ${CMAKE_CURRENT_LIST_DIR}/partition_a.ld)

# Enable USB output, disable UART
pico_enable_stdio_usb(partition_a 1)
pico_enable_stdio_uart(partition_a 0)

# Create map file
pico_add_extra_outputs(partition_a)

# ============================================================================
# PARTITION B - New Features
# ============================================================================

add_executable(partition_b
    partition_b.c
)

# Link libraries
target_link_libraries(partition_b
    pico_stdlib
    hardware_watchdog
)

# Use custom linker script for partition B
pico_set_linker_script(partition_b ${CMAKE_CURRENT_LIST_DIR}/partition_b.ld)

# Enable USB output, disable UART
pico_enable_stdio_usb(partition_b 1)
pico_enable_stdio_uart(partition_b 0)

# Create map file
pico_add_extra_outputs(partition_b)

# ============================================================================
# CUSTOM TARGETS FOR FLASHING
# ============================================================================

# Flash bootloader to device
add_custom_target(flash_bootloader
    COMMAND picotool load ${CMAKE_CURRENT_BINARY_DIR}/bootloader.uf2 -f
    DEPENDS bootloader
    COMMENT "Flashing bootloader to device..."
)

# Flash partition A to device
add_custom_target(flash_partition_a
    COMMAND picotool load ${CMAKE_CURRENT_BINARY_DIR}/partition_a.uf2 -o 0x40000 -f
    DEPENDS partition_a
    COMMENT "Flashing partition A to device (offset 0x40000)..."
)

# Flash partition B to device
add_custom_target(flash_partition_b
    COMMAND picotool load ${CMAKE_CURRENT_BINARY_DIR}/partition_b.uf2 -o 0x120000 -f
    DEPENDS partition_b
    COMMENT "Flashing partition B to device (offset 0x120000)..."
)

# Flash everything
add_custom_target(flash_all
    DEPENDS flash_bootloader flash_partition_a flash_partition_b
    COMMENT "Flashing all partitions to device..."
)

# ============================================================================
# SIZE REPORTING
# ============================================================================

# Report sizes after build
add_custom_command(TARGET bootloader POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:bootloader>
    COMMENT "Bootloader size:"
)

add_custom_command(TARGET partition_a POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:partition_a>
    COMMENT "Partition A size:"
)

add_custom_command(TARGET partition_b POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:partition_b>
    COMMENT "Partition B size:"
)

# ============================================================================
# COMPILER WARNINGS
# ============================================================================

# Enable all warnings
target_compile_options(bootloader PRIVATE -Wall -Wextra)
target_compile_options(partition_a PRIVATE -Wall -Wextra)
target_compile_options(partition_b PRIVATE -Wall -Wextra)

# ============================================================================
# COMPILE DEFINITIONS
# ============================================================================

# Add compile definitions for debugging
target_compile_definitions(bootloader PRIVATE
    PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=3000
)

target_compile_definitions(partition_a PRIVATE
    PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=3000
    PARTITION_A=1
)

target_compile_definitions(partition_b PRIVATE
    PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=3000
    PARTITION_B=1
)