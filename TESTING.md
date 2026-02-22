# Samwise Unit Testing Guide

This document explains how to write and run unit tests for Samwise that run locally (non-embedded) on your development machine.

## Overview

The testing framework allows you to test satellite code logic without needing embedded hardware. It works by:
- Replacing hardware-specific headers (logger, neopixel, etc.) with mock implementations
- Providing a simple `samwise_test()` function for creating tests

## Quick Start

### Building and Running Tests

```bash
bazel test //src/...

# Or run a specific test directly
bazel test //src/tasks/print:print_test
```

### Building for Embedded (unchanged)

```bash
# This still works exactly as before
bazel build :samwise --config=picubed-debug
```

## Writing a New Test

### 1. Write Your Test Code

Create a test file using **normal includes** - no special mock headers needed:

```c
// src/tasks/my_task/test/my_task_test.c
#include "my_task.h"  // Normal include - CMake handles routing to mocks
#include <string.h>

slate_t test_slate;

int main()
{
    LOG_DEBUG("Testing my_task");
    ASSERT(my_task.name != NULL);

    my_task.task_init(&test_slate);
    my_task.task_dispatch(&test_slate);

    return 0;
}
```

### 2. Add Test to BUILD.bazel

In your test's PARENT directory's BUILD.bazel:

```bazel
# src/tasks/my_task/BUILD.bazel
samwise_test(
  name = "my_test",
  srcs = ["test/my_test.c"],
  deps = [
    ":my_task", # Should correspond with cc_library before
  ]
)
```

That's it! The `samwise_test()` helper handles all the Bazel complexity:
- Links against test mocks automatically
- Sets up include paths correctly
- Registers the test with Bazel

## Available Mocks

The following embedded dependencies are automatically mocked in test mode:

### Driver & Module Mocks

| Real Header | Mock Location | Functionality |
|---|---|---|
| `adcs_driver.h` | adcs_driver_mock.c | Stubs ADCS init, power, and telemetry |
| `adm1176.h` | adm1176_mock.c | Returns fixed voltage/current readings |
| `burn_wire.h` | burn_wire_mock.c | Stubs burn wire init and activation |
| `device_status.h` | device_status_mock.c | Stubs solar/panel/RBF status queries |
| `logger.h` | logger_mock.c | Redirects log output to stdout/viz file |
| `mppt.h` | mppt_mock.c | Returns fixed MPPT voltage/current values |
| `mram.h` | mram_mock.c | In-memory MRAM read/write/allocation emulation |
| `neopixel.h` | neopixel_mock.c | Stubs NeoPixel RGB color setting |
| `onboard_led.h` | onboard_led_mock.c | Stubs onboard LED set/get/toggle |
| `payload_uart.h` | payload_uart_mock.c | Stubs payload UART read/write/power control |
| `rfm9x.h` | rfm9x_mock.c | Stubs RFM9x radio TX/RX/FIFO operations |
| `watchdog.h` | watchdog_mock.c | Stubs watchdog init and feed |
| `error.h` | error_mock.c | Prints fatal error and calls `exit(1)` |
| `state_ids.h` / `state_machine.h` | state_mock.c | Defines stub scheduler states with no-op transitions |

### Pico SDK Mocks (test_mocks)

| Real Header | Mock Location | Functionality |
|---|---|---|
| `hardware/flash.h` | flash.h | Pico flash memory API (no-op) |
| `hardware/gpio.h` | gpio.h | GPIO pin control (no-op) |
| `hardware/i2c.h` | i2c.h | I2C bus API (no-op) |
| `hardware/pwm.h` | pwm.h | PWM output API (no-op) |
| `hardware/resets.h` | resets.h | Hardware resets API (no-op) |
| `hardware/spi.h` | spi.h | SPI bus API (no-op) |
| `hardware/sync.h` | sync.h | Hardware synchronization primitives (no-op) |
| `hardware/uart.h` | uart.h | UART serial API (no-op) |
| `pico/stdlib.h` | stdlib.h | Pico standard library functions (no-op) |
| `pico/time.h` | time.h + `time.c` | Timing functions (provides `mock_time_us`) |
| `pico/types.h` | types.h | Pico SDK type definitions |
| `pico/unique_id.h` | unique_id.h | Board unique ID API (no-op) |
| `pico/util/queue.h` | queue.h | Pico queue utility API (no-op) |

## How It Works
// TODO: help from Yiheng :)
### Include Path Magic

When you write `#include "logger.h"` in your test, CMake's include path ordering determines which file gets included:

```cmake
target_include_directories(my_test PRIVATE
    ${PROJECT_SOURCE_DIR}/src/test_mocks      # Checked FIRST
    ${PROJECT_SOURCE_DIR}/src/drivers/logger  # Real headers here, checked second
)
```

Since `src/test_mocks/` comes first, the mock `logger.h` is found and used instead of the real embedded one.

### No Wrapper Headers Needed
// TODO: help from Yiheng :)
Unlike some test frameworks, you don't need to:
- Create wrapper headers in your test directory
- Use special mock-specific includes
- Modify your source code with `#ifdef TEST`

Just write normal includes and let CMake handle the routing!

## Best Practices

1. **Keep tests simple** - Test logic, not hardware
2. **One test per file** - Easier to debug
3. **Use meaningful names** - `my_task_test.c`, not `test1.c`
4. **Test return values** - Use `ASSERT()` to verify behavior
5. **Initialize state** - Create a fresh `slate_t` for each test

## Troubleshooting

### Test won't compile - "undefined reference"

Make sure your task library is properly linked in the test:
```cmake
samwise_add_test(
  NAME my_test
  SOURCES my_test.c
  LIBRARIES my_task  # Don't forget this!
)
```

### Wrong header being included

Check that test_mocks is in your include path and comes FIRST:
```cmake
target_include_directories(my_task PUBLIC
    "${PROJECT_SOURCE_DIR}/src/test_mocks"  # Must be first!
    "${PROJECT_SOURCE_DIR}/src/tasks/my_task"
)
```

### CTest doesn't find my test

Make sure you're using `samwise_add_test()` - it automatically registers with CTest.

## Example: Complete Test Setup

See `src/tasks/print/` for a complete working example:
- `print_task.c` - Task implementation (unchanged)
- `print_task.h` - Task header (unchanged)
- `test/print_test.c` - Test code using normal includes
- `test/CMakeLists.txt` - Simple test registration
- `CMakeLists.txt` - Task build with TEST/non-TEST branches
