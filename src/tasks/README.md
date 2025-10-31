# Tasks

This directory contains all the flight software tasks that run in different states of the satellite.

## Task LED Color Mapping

Each task has a unique neopixel LED color that lights up during task execution:

| Task | Color | RGB Values |
|------|-------|------------|
| Print | Cyan | (0, 255, 255) |
| Watchdog | Red | (255, 0, 0) |
| Telemetry | Blue | (0, 0, 255) |
| Beacon | Yellow | (255, 255, 0) |
| Radio | Magenta | (255, 0, 255) |
| Command | Orange | (255, 165, 0) |
| Payload | Purple | (128, 0, 128) |
| Burn Wire | Bright White | (255, 255, 255) |
| ADCS | Lime Green | (128, 255, 0) |
| Filesys | Pink | (255, 20, 147) |

## Adding a New Task

To add a new task to the flight software, follow these steps:

### 1. Create Task Directory Structure
```bash
mkdir src/tasks/your_task_name
```

### 2. Create Header File (`your_task_name.h`)
Use `print_task.h` as a template:

```c
#pragma once

#include "macros.h"
#include "slate.h"
#include "state_machine.h"
#include "typedefs.h"

// LED Color for your_task - Choose a unique color
#define YOUR_TASK_COLOR 128, 255, 0  // Example: Lime Green

void your_task_init(slate_t *slate);
void your_task_dispatch(slate_t *slate);

extern sched_task_t your_task;
```

### 3. Create Implementation File (`your_task_name.c`)
Use `print_task.c` as a template:

```c
#include "your_task_name.h"
#include "neopixel.h"

void your_task_init(slate_t *slate)
{
    LOG_INFO("Your task is initializing...");
    // Add initialization code here
}

void your_task_dispatch(slate_t *slate)
{
    neopixel_set_color_rgb(YOUR_TASK_COLOR);
    
    // Add your task logic here
    LOG_INFO("Your task is running...");
    
    neopixel_set_color_rgb(0, 0, 0);
}

sched_task_t your_task = {
    .name = "your_task",
    .dispatch_period_ms = 1000,  // Adjust period as needed
    .task_init = &your_task_init,
    .task_dispatch = &your_task_dispatch,
    .next_dispatch = 0
};
```

### 4. Create CMakeLists.txt
```cmake
add_library(your_task your_task_name.c)

# Specify targets to link compiled binaries
target_link_libraries(your_task PUBLIC slate common logger neopixel)

# Specify directories with relevant header files
target_include_directories(your_task PUBLIC
  "${PROJECT_SOURCE_DIR}/src/tasks/your_task_name"
)
```

### 5. Add Task to State
Edit the appropriate state file (e.g., `src/states/running/running_state.c`) to include your task:

```c
#include "your_task_name.h"  // Add this include

// Add your task to the state definition
sched_state_t running_state = {
    .name = "running",
    .num_tasks = 9,  // Increment the count
    .task_list = {&print_task, &watchdog_task, &blink_task, &telemetry_task,
                  &beacon_task, &radio_task, &command_task, &payload_task,
                  &your_task},  // Add your task here
    .get_next_state = &running_get_next_state
};
```

**Note**: Due to the fixed array structure of `task_list[MAX_TASKS_PER_STATE]` in the `sched_state_t` definition, tasks must be defined inline within the state structure rather than as a separate array variable.

### 6. Update Parent CMakeLists.txt
Add your task to `src/tasks/CMakeLists.txt`:

```cmake
add_subdirectory(your_task_name)
```

And to the main `src/CMakeLists.txt`:

```cmake
target_link_libraries(samwise PUBLIC
    # ... other libraries ...
    your_task
)
```

### 7. Choose a Unique LED Color
When selecting an LED color for your task:
- Choose RGB values that are visually distinct from existing tasks
- Update the color mapping table in this README
- Use values between 0-255 for each RGB component
- Consider brightness - very bright colors may be hard to distinguish

### Example Colors for New Tasks:
- Lime Green: (128, 255, 0)
- Pink: (255, 20, 147)
- Turquoise: (64, 224, 208)
- Gold: (255, 215, 0)
- Indigo: (75, 0, 130)

## Task Guidelines

- Keep task execution time minimal to avoid blocking other tasks
- Use appropriate dispatch periods based on task requirements
- Always include LED color control at the start and end of dispatch functions
- Log important task activities for debugging
- Handle errors gracefully and log error conditions