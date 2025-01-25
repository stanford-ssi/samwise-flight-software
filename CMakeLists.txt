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
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico-sdk/pico_sdk_init.cmake)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.4.0")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.4.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

project(samwise C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Include libraries
add_subdirectory(src/drivers)
add_subdirectory(src/scheduler)
add_subdirectory(src/state_machine/states)
add_subdirectory(src/state_machine/tasks)

# Add executable. Default name is the project name, version 0.1
file(GLOB top CONFIGURE_DEPENDS "src/*.c")

add_executable(samwise ${top})

target_link_libraries(samwise PRIVATE
  scheduler
  state_machine_states
  state_machine_tasks
  drivers
  pico_stdlib
  hardware_spi
)

pico_set_program_name(samwise "samwise")
pico_set_program_version(samwise "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(samwise 0)
pico_enable_stdio_usb(samwise 1)

# Add the standard include files to the build
target_include_directories(samwise PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src/common # common headers
)

pico_add_extra_outputs(samwise)

