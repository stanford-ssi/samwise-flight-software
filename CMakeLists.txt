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

# Default build in Release mode (including pico-sdk hardware libs)
if(NOT DEFINED CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
  set(BUILD_RELEASE ON)
  set(BUILD_TEST OFF)
endif()

message("CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

# Buildtype condition macros
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(BUILD_RELEASE OFF)
  set(BUILD_TEST ON)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
  set(BUILD_RELEASE ON)
  set(BUILD_TEST OFF)
endif()

# Testing build configurations
if(BUILD_TEST)
  message("===== Samwise Project building in TESTING mode. =====")
  add_definitions(-DTEST=1)
# Release build configurations
elseif(BUILD_RELEASE)
  message("===== Samwise Project building in RELEASE mode. =====")
  add_definitions(-DTEST=0)
  set(PICO_BOARD pico CACHE STRING "Board type")

  # Pull in Raspberry Pi Pico SDK (must be before project)
  include(${CMAKE_CURRENT_LIST_DIR}/pico-sdk/pico_sdk_init.cmake)

  if (PICO_SDK_VERSION_STRING VERSION_LESS "1.4.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.4.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
  endif()
endif()

project(samwise C CXX ASM)

# Include testing library
include(CTest)

# Only include pico sdk libraries when building in Release mode
if(BUILD_RELEASE)
  # Initialise the Raspberry Pi Pico SDK
  pico_sdk_init()
endif()

# Include src
# Executable `samwise` built under src/
add_subdirectory(src)
