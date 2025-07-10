# Building executable

## Pre-requisite

Download and install gcc-arm-none-eabi compiler: https://github.com/stanford-ssi/samwise-flight-software/blob/main/docs/ONBOARDING.md#linux

## Building from source

### Embedded Build (for hardware deployment)
1. Update submodule files with `git submodule update --init --recursive`
2. Create build directory `mkdir build-embedded` and navigate into it
3. Configure with ARM toolchain: `cmake -DCMAKE_TOOLCHAIN_FILE=../pico-sdk/cmake/preload/toolchains/pico_arm_cortex_m33_gcc.cmake -DPROFILE=PICUBED-DEBUG ..`
4. Make and compile binary `make -j8`

### Test Build (for unit testing)
1. Create test build directory `mkdir build-test` and navigate into it  
2. Configure for tests: `cmake -DPROFILE=TEST ..`
3. Build tests: `make -j8`
4. Run tests: `../run_tests.sh` (from project root)

# Code structure

The layout of our flight software is organized into 4 main categories:
- **Hardware Abstraction Layer (HAL)** contained within the `/hal` subdirectory - provides hardware-independent interface for testing
- **Hardware drivers** contained within the `/drivers` subdirectory - hardware-specific implementations using HAL
- **Common headers and shared data structures** contained within the `/common`, `/slate`, `/packet`, and `/utils` subdirectories
- **Finite-State-Machine (FSM) main event loop** which is defined by:
    - `/scheduler`: the execution main loop that implements state transitions
    - `/states`: definition of states the satellite can be in
    - `/tasks`: implementation of various isolated tasks that can be performed in any state
    - Initial entrypoint state is defined statically as [`init_state`](https://github.com/stanford-ssi/samwise-flight-software/blob/3203a54866b889707dfc175f2ac8ba17fd0a4b93/src/scheduler/scheduler.c#L20)

```
SAMWISE-FLIGHT-SOFTWARE
├── src
│   ├── common 		        # common headers and utilities
│   ├── drivers 		# hardware-specific driver implementations
│   │   ├── adm1176		# power monitor driver
│   │   ├── burn_wire		# burn wire driver  
│   │   ├── flash		# flash storage driver
│   │   ├── logger		# logging driver (TEST mode support)
│   │   ├── mppt		# MPPT power management
│   │   ├── neopixel		# RGB LED driver
│   │   ├── onboard_led		# onboard LED driver (HAL abstracted, with tests)
│   │   ├── payload_uart	# payload UART communication
│   │   ├── rfm9x		# radio driver
│   │   ├── watchdog		# watchdog driver
│   │   └── CMakeLists.txt
│   ├── error			# hardware specific error behavior
│   ├── hal			# Hardware Abstraction Layer
│   │   ├── hal_interface.h	# HAL interface definitions
│   │   ├── hal_mock.c		# mock implementation for tests
│   │   ├── hal_pico.c		# Pico SDK implementation
│   │   ├── test/		# HAL unit tests
│   │   └── CMakeLists.txt
│   ├── init			# system initialization
│   ├── packet			# packet handling and authentication
│   │   ├── packet.c
│   │   ├── packet.h
│   │   ├── test/		# packet unit tests
│   │   └── CMakeLists.txt
│   ├── scheduler		# main event loop scheduler
│   │   ├── CMakeLists.txt
│   │   ├── scheduler.c
│   │   ├── scheduler.h
│   │   ├── state_machine.h
│   │   └── states.h
│   ├── slate			# global shared data structure
│   │   ├── CMakeLists.txt
│   │   ├── slate.c
│   │   └── slate.h
│   ├── states			# states in FSM
│   │   ├── bringup
│   │   ├── burn_wire
│   │   ├── init
│   │   ├── running
│   │   └── CMakeLists.txt
│   ├── tasks			# standalone executable tasks
│   │   ├── beacon
│   │   ├── blink
│   │   ├── burn_wire
│   │   ├── command
│   │   ├── diagnostics
│   │   ├── payload
│   │   ├── print
│   │   ├── radio
│   │   ├── telemetry
│   │   ├── watchdog
│   │   └── CMakeLists.txt
│   ├── utils			# utility functions and helpers
│   └── CMakeLists.txt
├── main.c			# main entry point into software
├── run_tests.sh		# test runner script
└── CMakeLists.txt
```

# Testing Framework

## Overview

The project uses a comprehensive unit testing framework with Hardware Abstraction Layer (HAL) for hardware-independent testing. Tests can be run without requiring actual hardware.

## Running Tests

### Quick Test Run
```bash
# From project root
./run_tests.sh
```

### Manual Test Build and Run
```bash
# Create test build
mkdir build-test && cd build-test
cmake -DPROFILE=TEST ..
make -j8

# Run individual tests
./src/hal/test/hal_test
./src/packet/test/packet_test  
./src/drivers/onboard_led/test/onboard_led_test
```

## Adding New Tests

### For HAL-Abstracted Drivers

1. **Create test directory structure:**
   ```bash
   mkdir src/drivers/your_driver/test
   ```

2. **Create test source file:** `src/drivers/your_driver/test/your_driver_test.c`
   ```c
   #include <assert.h>
   #include <stdio.h>
   #include "your_driver.h"
   #include "hal_interface.h"

   void test_basic_functionality() {
       // Initialize HAL for testing
       hal_init();
       
       // Test your driver functionality
       // Use hal_mock_* functions to verify hardware interactions
       
       printf("✓ Basic functionality test passed\n");
   }

   int main() {
       printf("=== Your Driver Tests ===\n");
       test_basic_functionality();
       printf("All tests passed!\n");
       return 0;
   }
   ```

3. **Create test CMakeLists.txt:** `src/drivers/your_driver/test/CMakeLists.txt`
   ```cmake
   add_executable(your_driver_test your_driver_test.c)
   target_link_libraries(your_driver_test PRIVATE your_driver hal)
   target_compile_definitions(your_driver_test PRIVATE TEST_MODE)
   ```

4. **Update driver CMakeLists.txt:** Add test subdirectory
   ```cmake
   # Add tests if in TEST mode
   if(PROFILE STREQUAL "TEST")
       add_subdirectory(test)
   endif()
   ```

5. **Update run_tests.sh:** Add your test to the test runner script

### For Non-HAL Components

1. Follow similar structure but link against appropriate dependencies
2. Use standard C assertions and printf for test output
3. Focus on testing business logic and data structures

### Testing Best Practices

- **Use HAL mock functions** to verify hardware interactions:
  ```c
  hal_mock_get_pin_value(pin)     // Get mocked pin state
  hal_mock_get_pin_direction(pin) // Get mocked pin direction
  hal_mock_reset()                // Reset mock state
  ```

- **Test edge cases** and error conditions
- **Keep tests focused** - one test function per feature
- **Use descriptive test names** and clear assertions
- **Add printf statements** for test progress visibility

## Contributing

Please view open issues and ongoing PRs before contributing. To contribute a new feature, please:

1. Raise a high-level issue describing what the problem is, what is to be implemented and why. Also include a list of milestones and what success looks like after implementing this feature.
2. Raise additional issues setting their parent issue to the above high-level issue for specific implementation steps. At the minimum, one issue should be raised for each milestone. Issues can be nested under milestones and more granular sub-issues should be raised for each aspect of your contribution (e.g. hardware integration testing, test runs, writing documentation).
3. Assign high-level issue to a core contributor ([@devyaoyh](https://github.com/devyaoyh), [@quackwifhat](https://github.com/quackwifhat), [@niklas-vainio](https://github.com/niklas-vainio), [@shetaye](https://github.com/shetaye)) for triaging before starting.
