# Generated Cmake Pico project file
cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# by default, we are not testing
add_definitions(-DTEST=0)


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

# if(NOT TEST)
pico_set_program_name(samwise "samwise")
pico_set_program_version(samwise "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(samwise 0)
pico_enable_stdio_usb(samwise 1)

# Add the standard library to the build
target_link_libraries(samwise
        pico_stdlib)
# endif()

# Add the standard include files to the build
target_include_directories(samwise PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts or any other standard includes, if required
)

target_include_directories(samwise PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)

if(NOT TEST)
  pico_add_extra_outputs(samwise)
endif()



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
    
    # Add test sources
    file(GLOB test_sources CONFIGURE_DEPENDS "tests/*.c")
    add_executable(test_${PROJECT_NAME} ${test_sources} ${top} ${scheduler} 
        ${state_machine_tasks} ${state_machine_states} ${mock_pico})

    # Include directories for tests
    target_include_directories(test_${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/tests)

    # Handle CUnit for different platforms
    if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
        # Common Homebrew paths for Intel and M1 Macs
        list(APPEND CMAKE_PREFIX_PATH 
            "/usr/local"
            "/opt/homebrew"
            "/opt/local"
        )
        
        find_path(CUNIT_INCLUDE_DIR NAMES CUnit/CUnit.h
            PATHS ${CMAKE_PREFIX_PATH}
            PATH_SUFFIXES include
        )
        find_library(CUNIT_LIBRARY NAMES cunit libcunit CUnit
            PATHS ${CMAKE_PREFIX_PATH}
            PATH_SUFFIXES lib
        )

        if(NOT CUNIT_LIBRARY)
            message(FATAL_ERROR "CUnit library not found. Please ensure CUnit is installed: brew install cunit")
        endif()

        include_directories(${CUNIT_INCLUDE_DIR})
        target_link_libraries(test_${PROJECT_NAME} ${CUNIT_LIBRARY} m)
    else()
        # For Linux and others
        find_library(CUNIT_LIB CUnit)
        if(NOT CUNIT_LIB)
            message(FATAL_ERROR "CUnit not found. Please install CUnit development package")
        endif()
        target_link_libraries(test_${PROJECT_NAME} ${CUNIT_LIB} m)
    endif()
endif()
# find_package(CUnit REQUIRED)

# # Add include directories for CUnit
# include_directories(${CUNIT_INCLUDE_DIRS})

# # Link CUnit with your test target
# add_executable(test_${PROJECT_NAME} ${test_sources} ${top} ${scheduler} 
#         ${state_machine_tasks} ${state_machine_states} ${mock_pico})
# target_link_libraries(test_${PROJECT_NAME} ${CUNIT_LIB} m)
