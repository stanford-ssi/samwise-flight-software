# Samwise Testing Guide

This document explains how to write and run tests for Samwise. There are two kinds of tests:

| Kind                 | Runs on                       | Macro                        | Purpose                                   |
| -------------------- | ----------------------------- | ---------------------------- | ----------------------------------------- |
| **Unit test**        | Host (your laptop)            | `samwise_test()`             | Fast logic tests using mock drivers       |
| **Integration test** | Real hardware (RP2040/RP2350) | `samwise_integration_test()` | End-to-end tests against real peripherals |

Both are defined with Bazel macros in `//bzl:defs.bzl`.

---

## Quick Start

### Running Unit Tests (host)

```bash
# Run every unit test
bazel test //src/...

# Run a specific test
bazel test //src/tasks/print:print_test
```

### Building Integration Tests (hardware)

Integration tests are compiled into the **bringup** or **pico** firmware image. 
They are executed on-device by the `hardware_test_task` scheduler task.

```bash
# Build the bringup firmware (includes all registered integration tests)
bazel build :samwise --config=picubed-bringup
bazel build :samwise --config=pico # This works too
```

---

## Unit Tests with `samwise_test()`

### 1. Write Your Test Code

Create a test file using **normal includes** — no special mock headers needed:

```c
// src/tasks/my_task/test/my_task_test.c
#include "my_task.h"
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

### 2. Add the Test to `BUILD.bazel`

```starlark
load("//bzl:defs.bzl", "samwise_test")

samwise_test(
    name = "my_task_test",
    srcs = ["test/my_task_test.c"],
    deps = [
        ":my_task",
    ],
)
```

The `samwise_test()` macro handles everything automatically:

- Remaps driver and Pico SDK dependencies to mock implementations
- Adds `//src/test_infrastructure`
- Defines `TEST=1` for conditional compilation
- Restricts the target to the host platform

---

## Integration Tests with `samwise_integration_test()`

Integration tests exercise real hardware. They are compiled into a
`_hw_lib` library that gets linked into the bringup and pico firmware, where
`hardware_test_task` calls each test in sequence.

### How It Works

1. **`samwise_integration_test()`** creates a `cc_library` named
   `<name>_hw_lib`. It produces two kinds of compiled objects:
   - The **integration entry point** (`int_src`) should contain `<name>_int_main` 
     as a function, giving it a unique symbol (e.g. `mram_test_int_main`) that 
     `hardware_test_task` calls at runtime.
   - Any **shared helper sources** (`srcs`) are compiled with
     `-Dmain=_unused_<name>_main_`, which discards their `main()` so it is
     never linked or called — letting you reuse the same test file in both
     `samwise_test()` and `samwise_integration_test()` without conflicts.

2. **`hardware_integration_test_suite()`** collects all `_hw_lib` targets and
   auto-generates `hardware_tests.h` at build time (via
   `//bzl:gen_hw_tests_header`). The generated header declares every
   `<name>_int_main()` entry point and defines `HW_TEST_TABLE`, a struct array
   consumed by `hardware_test_task.c`.

3. **`hardware_test_task.c`** iterates over `HW_TEST_TABLE` and runs each test.
   No manual edits to the task or the header are needed when tests are added or
   removed — just update the `tests` dict.

### Step-by-Step: Adding a New Integration Test

#### 1. Write your test functions

Create a shared test file with the test logic. This file can (and usually
should) also be used by a corresponding `samwise_test()` unit test:

```c
// src/drivers/my_driver/test/my_driver_test.c
#include "my_driver_test.h"

void test_my_driver_read_write(void)
{
    uint8_t buf[16] = {0};
    my_driver_write(buf, sizeof(buf));
    my_driver_read(buf, sizeof(buf));
    ASSERT(buf[0] == 0xAB);
}
```

#### 2. Write the integration entry point

Create a separate `*_integration_test.c` file that calls into the shared tests.
Write it with a `<name>_int_main` so that `hardware_test_task` can call it at
runtime:

```c
// src/drivers/my_driver/test/my_driver_integration_test.c
#include "my_driver_test.h"

void my_driver_test_int_main(void)
{
    LOG_DEBUG("Starting my_driver integration tests\n");
    my_driver_init();
    test_my_driver_read_write();
    LOG_DEBUG("All my_driver tests passed!\n");
}
```

#### 3. Register both tests in `BUILD.bazel`

```starlark
# src/drivers/my_driver/test/BUILD.bazel
load("//bzl:defs.bzl", "samwise_integration_test", "samwise_test")

_DEPS = [
    "//src/common",
    "//src/drivers/logger",
    "//src/drivers/my_driver",
    "@pico-sdk//src/rp2_common/pico_stdlib",
]

# Host unit test (uses mocks)
samwise_test(
    name = "my_driver_test",
    srcs = ["my_driver_test.c", "my_driver_test.h"],
    deps = _DEPS,
)

# Hardware integration test (uses real drivers)
samwise_integration_test(
    name = "my_driver_test",
    int_src = "my_driver_integration_test.c",
    srcs = ["my_driver_test.c"],       # shared helper — its main() is discarded
    hdrs = ["my_driver_test.h"],
    deps = _DEPS + ["//src/tasks/hardware_test:hardware_test_assert"],
)
```

> **Note:** `samwise_test` and `samwise_integration_test` can share the same
> `name` because they produce differently-suffixed targets (`my_driver_test`
> vs `my_driver_test_hw_lib`) and are compiled at different times (i.e.)
> `bazel test` vs `bazel build`.

> **Note:** Helper sources passed via `srcs` may contain their own standalone
> `main()`. The macro compiles them with `-Dmain=_unused_<name>_main_` so
> their `main()` is discarded and never linked — no `#ifdef` guards needed.
> This is what lets the same `.c` file serve as the unit test entry point
> (via `samwise_test()`) and as a helper library (via `samwise_integration_test()`).

#### 4. Add the test to the suite

In `src/tasks/hardware_test/BUILD.bazel`, add your new `_hw_lib` target to the
`tests` dict:

```starlark
hardware_integration_test_suite(
    name = "hardware_test_lib",
    tests = {
        "mram_test":      "//src/filesys/test:mram_test_hw_lib",
        "filesys_test":   "//src/filesys/test:filesys_test_hw_lib",
        "my_driver_test": "//src/drivers/my_driver/test:my_driver_test_hw_lib",  # NEW
    },
    extra_deps = [
        "//src/common",
        "//src/drivers/logger",
        # ... any additional shared link-time deps ...
    ],
)
```

That's it! The next bringup or pico build will automatically include your test.

### Using `hardware_test_assert`

Integration tests run on hardware where a `fatal_error()` would halt the MCU
and prevent remaining tests from executing. The
`//src/tasks/hardware_test:hardware_test_assert` library provides a non-fatal
`ASSERT` override that logs failures via `LOG_ERROR` instead of aborting, so the
full test suite always runs to completion.

Add it to your integration test's `deps`:

```starlark
deps = _DEPS + ["//src/tasks/hardware_test:hardware_test_assert"],
```

---

## Available Mocks

The following embedded dependencies are automatically mocked when using
`samwise_test()`:

### Driver & Module Mocks

| Real Header                       | Mock Location        | Functionality                                        |
| --------------------------------- | -------------------- | ---------------------------------------------------- |
| `adcs_driver.h`                   | adcs_driver_mock.c   | Stubs ADCS init, power, and telemetry                |
| `adm1176.h`                       | adm1176_mock.c       | Returns fixed voltage/current readings               |
| `burn_wire.h`                     | burn_wire_mock.c     | Stubs burn wire init and activation                  |
| `device_status.h`                 | device_status_mock.c | Stubs solar/panel/RBF status queries                 |
| `logger.h`                        | logger_mock.c        | Redirects log output to stdout/viz file              |
| `mppt.h`                          | mppt_mock.c          | Returns fixed MPPT voltage/current values            |
| `mram.h`                          | mram_mock.c          | In-memory MRAM read/write/allocation emulation       |
| `neopixel.h`                      | neopixel_mock.c      | Stubs NeoPixel RGB color setting                     |
| `onboard_led.h`                   | onboard_led_mock.c   | Stubs onboard LED set/get/toggle                     |
| `payload_uart.h`                  | payload_uart_mock.c  | Stubs payload UART read/write/power control          |
| `rfm9x.h`                         | rfm9x_mock.c         | Stubs RFM9x radio TX/RX/FIFO operations              |
| `watchdog.h`                      | watchdog_mock.c      | Stubs watchdog init and feed                         |
| `error.h`                         | error_mock.c         | Prints fatal error and calls `exit(1)`               |
| `state_ids.h` / `state_machine.h` | state_mock.c         | Defines stub scheduler states with no-op transitions |

### Pico SDK Mocks (test_mocks)

| Real Header         | Mock Location     | Functionality                               |
| ------------------- | ----------------- | ------------------------------------------- |
| `hardware/flash.h`  | flash.h           | Pico flash memory API (no-op)               |
| `hardware/gpio.h`   | gpio.h            | GPIO pin control (no-op)                    |
| `hardware/i2c.h`    | i2c.h             | I2C bus API (no-op)                         |
| `hardware/pwm.h`    | pwm.h             | PWM output API (no-op)                      |
| `hardware/resets.h` | resets.h          | Hardware resets API (no-op)                 |
| `hardware/spi.h`    | spi.h             | SPI bus API (no-op)                         |
| `hardware/sync.h`   | sync.h            | Hardware synchronization primitives (no-op) |
| `hardware/uart.h`   | uart.h            | UART serial API (no-op)                     |
| `pico/stdlib.h`     | stdlib.h          | Pico standard library functions (no-op)     |
| `pico/time.h`       | time.h + `time.c` | Timing functions (provides `mock_time_us`)  |
| `pico/types.h`      | types.h           | Pico SDK type definitions                   |
| `pico/unique_id.h`  | unique_id.h       | Board unique ID API (no-op)                 |
| `pico/util/queue.h` | queue.h           | Pico queue utility API (no-op)              |

## How It Works

### Mock Substitution (Unit Tests)

The `samwise_test()` macro uses a mapping table (`_MOCK_MAPPINGS` in
`bzl/defs.bzl`) to rewrite dependency labels at analysis time. When your test
declares a dependency like `//src/drivers/rfm9x`, the macro transparently
replaces it with `//src/drivers/rfm9x:rfm9x_mock`. This means:

- Your test source files use the same `#include` paths as production code.
- No wrapper headers, special include directories, or `#ifdef TEST` guards are
  needed.
- The mock implementations are linked instead of the real hardware drivers.

### Integration Test Symbol Renaming

`samwise_integration_test()` keeps the **unit test** entry point and the
**integration test** entry point completely separate:

- **Shared helper sources** (`srcs`) — compiled with
  `-Dmain=_unused_<name>_main_`, which discards any `main()` they contain
  so it is never linked or called. This lets you reuse the same test `.c` file
  in both `samwise_test()` (where its `main()` _is_ the entry point) and
  `samwise_integration_test()` (where it is not).
- **Integration entry point** (`int_src`) — compiled with
  `-Dmain=<name>_int_main`, giving its `main()` a unique symbol like
  `mram_test_int_main()`. This is the function that `hardware_test_task` calls
  at runtime.

Because each integration test gets its own `_int_main` symbol, multiple tests
can coexist in the same firmware binary without `main()` conflicts.

### Auto-Generated `hardware_tests.h`

`hardware_integration_test_suite()` runs `bzl/gen_hw_tests_header.py` as a
Bazel genrule. The script takes each test name as a positional argument and
outputs a header that:

1. Declares `void <name>_int_main(void);` for every test.
2. Defines `HW_TEST_TABLE`, a compile-time array of `{name, function_pointer}`
   entries that `hardware_test_task.c` iterates over.

This means **no hand-maintained header** — adding or removing a test is a
one-line edit in the `tests` dict.

## Best Practices

1. **Keep tests simple** — test logic, not hardware (for unit tests)
2. **One test per file** — easier to debug
3. **Use meaningful names** — `my_task_test.c`, not `test1.c`
4. **Use `ASSERT()`** to verify behavior
5. **Initialize state** — create a fresh `slate_t` for each test
6. **Share test logic** — write core test functions in a shared `.c`/`.h` pair
   and reuse them in both `samwise_test()` and `samwise_integration_test()`
7. **Add `hardware_test_assert`** to integration test deps so failures don't
   halt the MCU

## Troubleshooting

### Test won't compile — "undefined reference"

Make sure your dependency is listed in `deps`:

```starlark
samwise_test(
    name = "my_test",
    srcs = ["my_test.c"],
    deps = [
        ":my_task",  # Don't forget this!
    ],
)
```

### Wrong mock being linked

Check that the dependency label in your `deps` exactly matches a key in
`_MOCK_MAPPINGS` (in `bzl/defs.bzl`). Both the short-form (`//src/drivers/rfm9x`)
and explicit-target form (`//src/drivers/rfm9x:rfm9x`) are mapped.

### Integration test not running on hardware

1. Verify `samwise_integration_test()` produces a `<name>_hw_lib` target
   (`bazel query //path/to:my_test_hw_lib`).
2. Verify the `_hw_lib` target is listed in the `tests` dict of
   `hardware_integration_test_suite()` in
   `src/tasks/hardware_test/BUILD.bazel`.
3. Build with a bringup or pico config: `bazel build :samwise --config=picubed-bringup`,
   `bazel build :samwise --config=pico`.

## Example: Complete Test Setup

See `src/filesys/test/` for a complete working example:

- `mram_test.c` / `mram_test.h` — shared test functions
- `mram_integration_test.c` — hardware entry point (`main()` → `mram_test_int_main()`)
- `BUILD.bazel` — declares both `samwise_test` and `samwise_integration_test`
- `src/tasks/hardware_test/BUILD.bazel` — registers the `_hw_lib` in the suite
