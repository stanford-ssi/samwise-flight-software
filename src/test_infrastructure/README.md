# Test Infrastructure

Shared test utilities for testing scheduler states with real scheduler code.

## Overview

This library provides common infrastructure for writing time-based tests for different scheduler states (running_state, burn_wire_state, future states, etc.). It eliminates code duplication and provides a consistent testing approach.

## Features

### Task Execution Tracking
- `get_task_stats(name)` - Get or create task statistics tracker
- `reset_task_stats()` - Reset all statistics for new test
- Tracks init count, dispatch count, and last dispatch time per task

### Visualization Logging
- `viz_log_open(filename)` - Open JSON log file for test visualization
- `viz_log_close()` - Close log file with proper formatting
- `log_viz_event(type, task, details)` - Log an event with timestamp
- Compatible with `/web/visualizer/` React app

### Test State Helpers
- `test_state_init_tasks(state, slate)` - Initialize all tasks in a state
- `run_scheduler_simulation(slate, duration_ms, interval_ms, log_interval_ms)` - Run scheduler loop with time advancement
- `verify_dispatch_count(task, expected, tolerance)` - Verify task executed expected number of times
- `log_discovered_tasks(state)` - Log all tasks to visualization
- `log_task_summary()` - Log execution counts for all tasks

## Usage Example

```c
#include "test_scheduler_helpers.h"
#include "scheduler.h"

// Define your test tasks
void my_task_init(slate_t *slate) {
    task_execution_stats_t *stats = get_task_stats("my_task");
    if (stats) stats->init_count++;
    log_viz_event("task_init", "my_task", "Testing task");
}

void my_task_dispatch(slate_t *slate) {
    task_execution_stats_t *stats = get_task_stats("my_task");
    if (stats) {
        stats->dispatch_count++;
        stats->last_dispatch_time_ms = (uint32_t)(mock_time_us / 1000);
    }
    log_viz_event("task_dispatch", "my_task", "dispatch");
}

sched_task_t my_task = {
    .name = "my_task",
    .dispatch_period_ms = 100,
    .task_init = my_task_init,
    .task_dispatch = my_task_dispatch
};

// Define your test state
sched_state_t my_test_state = {
    .name = "my_test_state",
    .num_tasks = 1,
    .task_list = {&my_task},
    .get_next_state = ...
};

// Write your test
void test_my_state() {
    // Setup
    mock_time_us = 0;
    reset_task_stats();
    slate_t slate = {0};
    slate.current_state = &my_test_state;
    
    // Open visualization log
    viz_log_open("my_test.json");
    
    // Initialize tasks
    test_state_init_tasks(&my_test_state, &slate);
    
    // Log discovered tasks
    log_discovered_tasks(&my_test_state);
    
    // Run simulation (10 seconds, check every 5ms, log every 1000ms)
    run_scheduler_simulation(&slate, 10000, 5, 1000);
    
    // Verify results
    ASSERT(verify_dispatch_count("my_task", 100, 5));
    
    // Close log
    viz_log_close();
}
```

## Integration

Add to your test's CMakeLists.txt:

```cmake
samwise_add_test(
  NAME my_state_test
  SOURCES test_my_state.c ${PROJECT_SOURCE_DIR}/src/scheduler/scheduler.c
  LIBRARIES test_infrastructure
)
```

## Stub States

The library declares extern stubs for common states that scheduler.c expects:
- `init_state`
- `running_state`
- `burn_wire_state`
- `burn_wire_reset_state`
- `bringup_state`

Define these in your test file as needed.

## Mock Time

Uses `mock_time_us` from `test_mocks/pico/stdlib.h` for time simulation.

## See Also

- `/src/states/running/test/test_running_state.c` - Example usage
- `/web/visualizer/` - Visualization tool for test logs
