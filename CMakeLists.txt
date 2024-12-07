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

# Add executable. Default name is the project name, version 0.1
file(GLOB top CONFIGURE_DEPENDS "src/*.c")
file(GLOB scheduler CONFIGURE_DEPENDS "src/scheduler/*.c")
file(GLOB state_machine_tasks CONFIGURE_DEPENDS "src/state_machine/tasks/*.c")
file(GLOB state_machine_states CONFIGURE_DEPENDS "src/state_machine/states/*.c")
file(GLOB drivers CONFIGURE_DEPENDS "src/drivers/*.c")

add_executable(samwise_code ${top} ${scheduler} ${state_machine_tasks}
  ${state_machine_states} ${drivers})

function(configure_samwise_executable TARGET_NAME DEFINES)
    add_executable(${TARGET_NAME} ${samwise_code})
    
    pico_set_program_name(${TARGET_NAME} ${TARGET_NAME})
    pico_set_program_version(${TARGET_NAME} "0.1")
    
    pico_enable_stdio_uart(${TARGET_NAME} 0)
    pico_enable_stdio_usb(${TARGET_NAME} 1)
    
    target_link_libraries(${TARGET_NAME} hardware_spi pico_stdlib)
    
    target_include_directories(${TARGET_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/..
        ${CMAKE_CURRENT_LIST_DIR}/src
    )

    target_compile_definitions(${TARGET_NAME} PRIVATE ${DEFINES})
    
    pico_add_extra_outputs(${TARGET_NAME})
endfunction()

# samwise: pico exectuable
# samwise_flight: picubed executable, flight
# samwise_bringup: picubed executable, no fight
configure_samwise_executable(samwise PICO)
configure_samwise_executable(samwise_flight PICO FLIGHT)
configure_samwise_executable(samwise_bringup BRINGUP)
