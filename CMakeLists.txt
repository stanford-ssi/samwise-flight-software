# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NEVER EDIT THE NEXT LINES for Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.0.0)
set(toolchainVersion 13_2_Rel1)
set(picotoolVersion 2.0.0)
include(${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
# ====================================================================================
option(RP2350 "Build for RP2350" OFF)
message(WARNING "Note: When swapping between pico 1 and pico 2, you may have to clear the cache (rm -rf build/*)")
if(RP2350)
    set(PICO_BOARD pico2)
else()
    set(PICO_BOARD pico)
endif()

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico-sdk/pico_sdk_init.cmake)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.4.0")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.4.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

project(samwise C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1
file(GLOB top CONFIGURE_DEPENDS "src/*.c")
file(GLOB scheduler CONFIGURE_DEPENDS "src/scheduler/*.c")
file(GLOB state_machine_tasks CONFIGURE_DEPENDS "src/state_machine/tasks/*.c")
file(GLOB state_machine_states CONFIGURE_DEPENDS "src/state_machine/states/*.c")
file(GLOB drivers CONFIGURE_DEPENDS "src/drivers/*.c")

set(samwise_code ${top} ${scheduler} ${state_machine_tasks}
  ${state_machine_states} ${drivers})

option(COMMIT_HASH "Commit hash" "no-hash")

function(configure_samwise_executable TARGET_NAME DEFINES)
    add_executable(${TARGET_NAME} ${samwise_code})
    
    pico_set_program_name(${TARGET_NAME} ${TARGET_NAME})
    pico_set_program_version(${TARGET_NAME} "0.1")
    
    pico_enable_stdio_uart(${TARGET_NAME} 0)
    pico_enable_stdio_usb(${TARGET_NAME} 1)
    
    target_link_libraries(${TARGET_NAME} hardware_spi hardware_i2c
      pico_unique_id pico_stdlib)
    
    target_include_directories(${TARGET_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/..
        ${CMAKE_CURRENT_LIST_DIR}/src
    )

    target_compile_definitions(${TARGET_NAME} PRIVATE ${DEFINES}
      COMMIT_HASH="${COMMIT_HASH}")
    
    pico_add_extra_outputs(${TARGET_NAME})
endfunction()

# samwise_pico_debug: pico exectuable
# samwise_picubed_debug: picubed executable, for debugging
# samwise_picubed_flight: picubed executable, for flight
# samwise_picubed_bringup: picubed executable, for bringing up the board
configure_samwise_executable(samwise_pico_debug PICO)
configure_samwise_executable(samwise_picubed_debug "")
configure_samwise_executable(samwise_picubed_flight FLIGHT)
configure_samwise_executable(samwise_picubed_bringup BRINGUP)
