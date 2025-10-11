# Samwise Unit Testing Guide

This document explains how to write and run unit tests for Samwise that run locally (non-embedded) on your development machine.

## Overview

The testing framework allows you to test satellite code logic without needing embedded hardware. It works by:
- Replacing hardware-specific headers (logger, neopixel, etc.) with mock implementations
- Using CMake include path ordering to automatically route includes to mocks
- Providing a simple `samwise_add_test()` function for creating tests

## Quick Start

### Building and Running Tests

```bash
# Configure for tests (uses native compiler, not ARM)
cmake -B build_tests -DPROFILE=TESTS

# Build all tests
cmake --build build_tests

# Run all tests with CTest
cd build_tests && ctest --output-on-failure

# Or run a specific test directly
./build_tests/src/tasks/print/test/print_task_test
```

### Building for Embedded (unchanged)

```bash
# This still works exactly as before
cmake -B build_embedded -DPROFILE=PICUBED_DEBUG
cmake --build build_embedded
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

### 2. Add Test to CMakeLists.txt

In your test directory's CMakeLists.txt:

```cmake
# src/tasks/my_task/test/CMakeLists.txt
samwise_add_test(
  NAME my_task_test
  SOURCES my_task_test.c
  LIBRARIES my_task
)
```

That's it! The `samwise_add_test()` helper handles all the CMake complexity:
- Links against test mocks automatically
- Sets up include paths correctly
- Registers the test with CTest

### 3. Update Your Task's CMakeLists.txt

If your task doesn't already support tests, add a BUILD_TESTS section:

```cmake
# src/tasks/my_task/CMakeLists.txt
if(BUILD_TESTS)
  # For test builds, create library that links against mocks
  add_library(my_task my_task.c)
  target_link_libraries(my_task PUBLIC test_mocks)
  target_include_directories(my_task PUBLIC
    "${PROJECT_SOURCE_DIR}/src/test_mocks"
    "${PROJECT_SOURCE_DIR}/src/tasks/my_task"
  )
  add_subdirectory(test)
else()
  # For embedded builds, use real hardware dependencies
  add_library(my_task my_task.c)
  target_link_libraries(my_task PUBLIC slate common logger neopixel)
  target_include_directories(my_task PUBLIC
    "${PROJECT_SOURCE_DIR}/src/tasks/my_task"
  )
endif()
```

## Available Mocks

The following embedded dependencies are automatically mocked in test mode:

| Real Header | Mock Location | Functionality |
|-------------|---------------|---------------|
| `logger.h` | `src/test_mocks/logger.h` | Prints log messages to stdout |
| `error.h` | `src/test_mocks/error.h` | Fatal error handling |
| `neopixel.h` | `src/test_mocks/neopixel.h` | LED control (no-op) |
| `slate.h` | `src/test_mocks/slate.h` | Satellite state structure |
| `state_machine.h` | `src/test_mocks/state_machine.h` | Scheduler types |
| `macros.h` | `src/test_mocks/macros.h` | Build macros (IS_TEST, etc.) |
| `typedefs.h` | `src/test_mocks/typedefs.h` | Common type definitions |

## How It Works

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
