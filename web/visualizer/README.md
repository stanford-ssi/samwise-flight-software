# SAMWISE FSM Visualizer

A web-based visualization tool for analyzing satellite state machine transitions and task execution across build profiles.

## Overview

This visualizer helps illustrate what the satellite FSM is doing during integration tests. It supports both single-state tests (e.g. running state) and multi-state FSM tests that exercise full transition paths. It provides:

- **State Machine Diagram**: FSM nodes and transition arrows showing the state flow
- **State Timeline**: Color-coded bar showing which state was active over time
- **Task Timeline**: Visual representation of when each task initializes and dispatches, with state background bands
- **Task List**: Overview of all discovered tasks with their dispatch periods
- **Event Log**: Detailed chronological log of all events (tasks, state transitions, milestones)

## Setup

1. Install dependencies:
   ```bash
   cd web/visualizer
   npm install
   ```

2. Start the development server:
   ```bash
   npm run dev
   ```

3. Open your browser to `http://localhost:3000`

## Generating Log Data

### FSM integration tests (recommended)

The FSM tests exercise the full state machine for each build profile. They are tagged
`manual` so they don't run with `bazel test //...` (which is reserved for unit tests).
Run them individually with their dedicated `.bazelrc` configs:

```bash
# Debug profile: init -> running (no extra config needed)
bazel test //src/scheduler:fsm_test_debug

# Flight profile: init -> burn_wire -> running
bazel test //src/scheduler:fsm_test_flight --config=fsm-flight

# Bringup profile: init -> bringup
bazel test //src/scheduler:fsm_test_bringup --config=fsm-bringup
```

Output JSON files are saved to:
```
bazel-testlogs/src/scheduler/fsm_test_debug/test.outputs/fsm_viz_picubed_debug.json
bazel-testlogs/src/scheduler/fsm_test_flight/test.outputs/fsm_viz_picubed_flight.json
bazel-testlogs/src/scheduler/fsm_test_bringup/test.outputs/fsm_viz_picubed_bringup.json
```

### Single-state running test

```bash
bazel test //src/states/running:running_state_test --config=tests
```

Upload the generated JSON file to the visualizer using the "Load Log File" button.

## Features

### State Machine Diagram
- Rounded-rectangle state nodes arranged in transition order
- Arrows showing observed transitions between states
- Final (stable) state highlighted with a dashed border
- Task count displayed per state

### State Timeline
- Horizontal bar showing which state was active at each point in time
- Color-coded per state

### Task Timeline
- Shows when each task is initialized and dispatched
- Color-coded tasks for easy identification
- State background bands showing which state was active behind the task rows
- Dashed vertical lines at state transition boundaries
- Time markers in milliseconds

### Task List
- Displays all discovered tasks
- Shows dispatch period for each task
- Click to filter the timeline and event log

### Event Log
- Scrollable list of all events
- Color-coded by event type
- Shows timestamps, task names, and details
- Toggle to show/hide task log output

## Event Types

The visualizer recognizes these event types:

| Event | Color | Description |
|-------|-------|-------------|
| `fsm_start` / `fsm_end` | Indigo | FSM test start/end with profile info |
| `state_enter` | Green | Entering a new state |
| `state_exit` | Orange | Leaving a state |
| `test_start` | Blue | Test begins |
| `test_pass` | Green | Test completes successfully |
| `task_discovered` | Purple | New task found in a state |
| `task_init` | Amber | Task initialization |
| `task_start` / `task_end` | Green/Gray | Task dispatch execution |
| `task_log` | Cyan | Log output from a task |
| `simulation_milestone` | Gray | Periodic progress marker |

## Building for Production

```bash
npm run build
npm run preview
```

This creates an optimized build in the `dist` directory.
