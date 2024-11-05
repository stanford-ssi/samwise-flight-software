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
if(NOT TEST)

  set(picotoolVersion 2.0.0)
  include(${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
  # ====================================================================================
  set(PICO_BOARD pico CACHE STRING "Board type")

  # Pull in Raspberry Pi Pico SDK (must be before project)
  include(pico-sdk/pico_sdk_init.cmake)

  if (PICO_SDK_VERSION_STRING VERSION_LESS "1.4.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.4.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
  endif()
endif()
project(samwise C CXX ASM)
if(NOT TEST)
  # Initialise the Raspberry Pi Pico SDK
  pico_sdk_init()
endif()

# Add executable. Default name is the project name, version 0.1
file(GLOB top CONFIGURE_DEPENDS "src/*.c")
file(GLOB scheduler CONFIGURE_DEPENDS "src/scheduler/*.c")
file(GLOB state_machine_tasks CONFIGURE_DEPENDS "src/state_machine/tasks/*.c")
file(GLOB state_machine_states CONFIGURE_DEPENDS "src/state_machine/states/*.c")
add_executable(samwise ${top} ${scheduler} ${state_machine_tasks}
  ${state_machine_states})

if(NOT TEST)
  pico_set_program_name(samwise "samwise")
  pico_set_program_version(samwise "0.1")

  # Modify the below lines to enable/disable output over UART/USB
  pico_enable_stdio_uart(samwise 0)
  pico_enable_stdio_usb(samwise 1)

  # Add the standard library to the build
  target_link_libraries(samwise
          pico_stdlib)
endif()

# Add the standard include files to the build
target_include_directories(samwise PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts or any other standard includes, if required
)

target_include_directories(samwise PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)

if(NOT TEST)
  pico_add_extra_outputs(samwise)
endif()

add_definitions(-DTEST)

if(TEST)
    # Remove any ARM-specific compiler settings
    set(CMAKE_C_COMPILER gcc)  # or clang for Mac
    set(CMAKE_CXX_COMPILER g++)  # or clang++ for Mac
    
    # Clear any ARM-specific flags
    set(CMAKE_C_FLAGS "")
    set(CMAKE_CXX_FLAGS "")
    
    # Add mock implementations
    file(GLOB mock_pico CONFIGURE_DEPENDS "mock_pico/*.c")
    target_sources(samwise PRIVATE ${mock_pico})
    target_include_directories(samwise PRIVATE ${CMAKE_CURRENT_LIST_DIR}/mock_pico)
    
    # Add test-specific compiler definitions
    add_definitions(-DTEST)
endif()
