# SAMWISE Flight Computer Code

Flight computer firmware for the SAMWISE 2U cubesat. Designed for the Raspberry
Pi RP2040 and RP2350 microcontrollers.

## Getting Started

See [the onboarding doc](docs/ONBOARDING.md) for development environment setup. To set some useful configurations for this repo, run `configure.sh`.

## Design Objectives

Beyond functional parity with the prior CircuitPython-based Sapling firmware,
this project has a few additional goals:
* Reliable OTAs
* Use the [blackboard pattern](https://en.wikipedia.org/wiki/Blackboard_(design_pattern))
* Eliminate heap allocations
* Simple flight/debug configuration
* Clean code

## Building
To build the code in this repo, run `cmake -B build -DPROFILE=PICUBED-DEBUG` then `cmake --build build --parallel`.

The following targets will be built:
* `samwise_pico_debug`: pico exectuable
* `samwise_picubed_debug`: picubed executable, for debugging
* `samwise_picubed_flight`: picubed executable, for flight
* `samwise_picubed_bringup`: picubed executable, for bringing up the board
(these can be configured in `CMakeLists.txt`)

### Build Archives
The **C Build** github action automatically builds RP2040 and RP2350 archives on pushes to pull requests into main.

## Switching build targets
To switch between builds for the RP2040 (Pico 1) and RP2350 (Pico 2 and Pycubed), set the `RP2350` flag to 1 when calling `cmake`. For example, to build for the RP2350, run:
```
cmake .. -DRP2350=1
```

**Note that when switching between the RP2040 and RP2350, it is usually necessary to clear the cmake cache (by running `rm -rf build/*`)**

## Testing

The project includes comprehensive unit and integration tests to ensure code quality and reliability. The test suite uses a Hardware Abstraction Layer (HAL) to enable testing without physical hardware.

### Test Architecture

The testing framework consists of two main components:

1. **Unit Tests**: Test individual functions and modules in isolation
2. **Integration Tests**: Test system-level behavior with mocked hardware

### Hardware Abstraction Layer (HAL)

The HAL provides a unified interface for hardware operations:
- **Embedded builds**: HAL calls map directly to Pico SDK functions (zero overhead)
- **Test builds**: HAL calls route through controllable mock implementations

### Running Tests

#### Prerequisites
- Standard C compiler (GCC or Clang)
- CMake 3.13+
- Standard C library development headers

#### Quick Start
```bash
# Run all tests
./run_tests.sh
```

#### Manual Test Execution
```bash
# Create test build directory
mkdir build-tests && cd build-tests

# Configure for host-native testing
cmake -DPROFILE=TEST ..

# Build tests
make

# Run individual tests
./src/hal/test/hal_test
./src/packet/test/packet_test
./src/tasks/beacon/test/beacon_task_test
./src/tasks/print/test/print_task_test
```

### Test Categories

#### Unit Tests
- **Packet Authentication** (`src/packet/test/`) - HMAC validation, replay protection
- **HAL Interface** (`src/hal/test/`) - Hardware abstraction layer functionality
- **Task Logic** (`src/tasks/*/test/`) - Individual task behavior

#### Integration Tests
- **Main Event Loop** - Full system execution with mocked hardware
- **State Machine** - State transitions and system behavior
- **Command Processing** - End-to-end command handling

### Test Output
Tests provide clear pass/fail indicators:
```
Running HAL Basic Tests... PASSED
Running Packet Authentication Tests... PASSED
✓ Packet structure layout correct
✓ Authentication disabled mode works
✓ NULL packet pointer handled correctly
```

### Writing New Tests

To add tests for a new module:

1. Create test directory: `src/your_module/test/`
2. Add test files:
   - `your_module_test.c` - Test implementation
   - `your_module_test.h` - Test headers
   - `CMakeLists.txt` - Build configuration

3. Example CMakeLists.txt:
```cmake
if(PROFILE STREQUAL "TEST")
    enable_testing()
    add_executable(your_module_test your_module_test.c)
    target_link_libraries(your_module_test your_module hal)
    add_test(NAME YourModuleTest COMMAND your_module_test)
endif()
```

4. Update parent CMakeLists.txt:
```cmake
add_subdirectory(test)
```

### Continuous Integration

Tests are automatically run in GitHub Actions on:
- Pull requests to main branch
- Pushes to main branch
- Manual workflow triggers

### Test Coverage

The test suite covers:
- **Security-critical functions**: Packet authentication, command validation
- **Hardware interfaces**: GPIO, SPI, I2C, UART operations
- **System behavior**: State machine, task scheduling
- **Error handling**: Invalid inputs, hardware failures

### Mock Hardware Features

The mock HAL provides:
- Controllable GPIO pin states
- Simulated SPI/I2C/UART transactions
- Configurable hardware failures
- Time advancement for timing tests
- Transaction counting for verification

Example mock usage:
```c
// Set up mock hardware state
hal_mock_set_pin_value(LED_PIN, false);
hal_mock_advance_time(1000);

// Run code under test
led_toggle();

// Verify behavior
assert(hal_mock_get_pin_value(LED_PIN) == true);
```

### Performance Testing

Tests can be run with timing analysis:
```bash
# Build with coverage and profiling
cmake -DPROFILE=TEST -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON ..
make
ctest

# Generate coverage report (if tools available)
gcov src/**/*.c
```
