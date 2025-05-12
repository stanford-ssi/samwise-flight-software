# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

option(PROFILE "Build profile to build for" "PICO")
option(COMMIT_HASH "Commit hash" "no-hash")
if(PROFILE STREQUAL "PICO")
  set(PICO_BOARD pico)
  set(SAMWISE_DEFINES PICO)
elseif(PROFILE STREQUAL "PICUBED-DEBUG")
  set(PICO_BOARD pico2)
  set(SAMWISE_DEFINES "")
elseif(PROFILE STREQUAL "PICUBED-FLIGHT")
  set(PICO_BOARD pico2)
  set(SAMWISE_DEFINES FLIGHT)
elseif(PROFILE STREQUAL "PICUBED-BRINGUP")
  set(PICO_BOARD pico2)
  set(SAMWISE_DEFINES BRINGUP)
elseif(PROFILE STREQUAL "TEST")
  set(SAMWISE_DEFINES TEST)
else()
  message(WARNING "Unrecognized build profile ${PROFILE}")
endif()

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

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico-sdk/pico_sdk_init.cmake)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.4.0")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.4.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

project(samwise C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Include src
# Executable `samwise` built under src/
add_subdirectory(src)
