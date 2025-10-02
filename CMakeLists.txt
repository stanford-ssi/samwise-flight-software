# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

# option(PROFILE "Build profile to build for" "PICO")
if(PROFILE STREQUAL "OFF")
  set(PROFILE PICO)
endif()

option(COMMIT_HASH "Commit hash" "no-hash")
option(PACKET_HMAC_PSK "Packet HMAC PSK")

set(PICO_BOARD_HEADER_DIRS ${CMAKE_CURRENT_LIST_DIR}/boards)

if(PROFILE STREQUAL "PICO")
  set(PICO_BOARD pico)
  set(SAMWISE_DEFINES PICO)
elseif(PROFILE STREQUAL "PICUBED-DEBUG")
  set(PICO_BOARD samwise_picubed)
  set(SAMWISE_DEFINES "")
elseif(PROFILE STREQUAL "PICUBED-FLIGHT")
  set(PICO_BOARD samwise_picubed)
  set(SAMWISE_DEFINES FLIGHT)
elseif(PROFILE STREQUAL "PICUBED-BRINGUP")
  set(PICO_BOARD samwise_picubed)
  set(SAMWISE_DEFINES BRINGUP)
elseif(PROFILE STREQUAL "TEST")
  set(SAMWISE_DEFINES TEST)
else()
  message(WARNING "Unrecognized build profile ${PROFILE}")
endif()

string(STRIP "${PACKET_HMAC_PSK}" PACKET_HMAC_PSK)
if(NOT DEFINED PACKET_HMAC_PSK OR PACKET_HMAC_PSK STREQUAL "OFF")
  message(WARNING "Running with authentication disabled")
  if(PROFILE STREQUAL "PICUBED-FLIGHT")
    message(FATAL_ERROR "PACKET_HMAC_PSK must be set for flight profile")
  endif()
else()
  string(LENGTH "${PACKET_HMAC_PSK}" hmac_psk_length)
  if(NOT hmac_psk_length EQUAL 32)
    message(FATAL_ERROR "PACKET_HMAC_PSK must be exactly 32 characters long, got ${hmac_psk_length} characters")
  endif()
  message(STATUS "Packet authentication code enabled")
  add_compile_definitions(PACKET_HMAC_PSK=\"${PACKET_HMAC_PSK}\")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
include(${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
# ====================================================================================

set(PICO_BOARD pico2 CACHE STRING "Board Type")

# Pull in Raspberry Pi Pico SDK (must be before project)
# include(pico-sdk/pico_sdk_init.cmake)
include(pico_sdk_import.cmake)

# if (PICO_SDK_VERSION_STRING VERSION_LESS "1.4.0")
#   message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.4.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
# endif()

project(samwise C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add the library directories
add_subdirectory(lib/tinycrypt)

# Include src
# Executable `samwise` built under src/
add_subdirectory(src)
