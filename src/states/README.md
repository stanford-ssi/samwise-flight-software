# States

This directory contains the FSM states for the SAMWISE satellite. Each state defines a set of tasks to run and a transition function.

## Current States

| State | Description | Build Profile |
|-------|-------------|---------------|
| `init` | Boot-up state, no tasks. Transitions immediately. | All |
| `running` | Normal operation with telemetry, radio, etc. | Default / Debug |
| `bringup` | Board bring-up with diagnostics. | `BRINGUP` only |
| `burn_wire` | Antenna deployment sequence. | `FLIGHT` only |
| `burn_wire_reset` | Post-deployment recovery. | `FLIGHT` only |

## Adding a New State

### 1. Add the state ID

Add an entry to the `state_id_t` enum in `src/scheduler/state_ids.h` (before `STATE_COUNT`):

```c
typedef enum
{
    STATE_NONE = -1,
    STATE_INIT = 0,
    STATE_RUNNING,
    STATE_BURN_WIRE,
    STATE_BURN_WIRE_RESET,
    STATE_BRINGUP,
    STATE_YOUR_STATE,  // <-- add here
    STATE_COUNT
} state_id_t;
```

### 2. Create the state directory and files

```bash
mkdir src/states/your_state
```

**Header** (`your_state.h`):
```c
#pragma once

#include "macros.h"
#include "slate.h"
#include "state_machine.h"
#include "typedefs.h"

// Include headers for tasks used in this state
#include "watchdog_task.h"
#include "your_task.h"

state_id_t your_state_get_next_state(slate_t *slate);
extern sched_state_t your_state;
```

**Implementation** (`your_state.c`):
```c
#include "your_state.h"

state_id_t your_state_get_next_state(slate_t *slate)
{
    // Return state ID to transition to (or STATE_YOUR_STATE to stay)
    return STATE_YOUR_STATE;
}

sched_state_t your_state = {
    .name = "your_state",
    .id = STATE_YOUR_STATE,
    .num_tasks = 2,
    .task_list = {&watchdog_task, &your_task},
    .get_next_state = &your_state_get_next_state
};
```

### 3. Create `BUILD.bazel`

```python
package(default_visibility = ["//visibility:public"])

cc_library(
    name = "your_state",
    srcs = ["your_state.c"],
    hdrs = ["your_state.h"],
    includes = ["."],
    deps = [
        "//src/common",
        "//src/slate",
        "//src/scheduler:state_machine",
        "//src/scheduler:state_ids",
        # Task dependencies
        "//src/tasks/watchdog:watchdog_task",
        "//src/tasks/your_task:your_task",
    ] + select({
        "//bzl:test_mode": [
            "//src/test_mocks:pico_stdlib_mock",
        ],
        "//conditions:default": [
            "@pico-sdk//src/rp2_common/pico_stdlib:pico_stdlib",
        ],
    }),
)
```

### 4. Register the state in the scheduler

In `src/scheduler/scheduler.c`, add:
```c
#include "your_state.h"

// In sched_init():
state_registry_register(STATE_YOUR_STATE, &your_state);
```

And add the dependency in `src/scheduler/BUILD.bazel`:
```python
"//src/states/your_state:your_state",
```

### 5. Add transitions

Update other states' `get_next_state()` functions to transition to `STATE_YOUR_STATE` when appropriate.
