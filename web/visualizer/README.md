# Satellite Running State Visualizer

A web-based visualization tool for analyzing satellite task execution during running state tests.

## Overview

This visualizer helps illustrate what the satellite is doing during the running state FSM tests. It provides:

- **Task Timeline**: Visual representation of when each task initializes and dispatches
- **Task List**: Overview of all discovered tasks with their dispatch periods
- **Event Log**: Detailed chronological log of all events

## Setup

1. Install dependencies:
   ```bash
   cd /code/web/visualizer
   npm install
   ```

2. Start the development server:
   ```bash
   npm run dev
   ```

3. Open your browser to `http://localhost:3000`

## Generating Log Data

The C test file has been modified to emit JSON log files. To generate visualization data:

1. Build and run the running state test:
   ```bash
   # From your build directory
   ./test_running_state
   ```

2. This will create a file named `running_state_viz.json` in the current directory

3. Upload this JSON file to the visualizer using the "Load Log File" button

## Features

### Task Timeline
- Shows when each task is initialized and dispatched
- Color-coded tasks for easy identification
- Time markers in milliseconds
- Visual indication of dispatch cycles

### Task List
- Displays all discovered tasks
- Shows dispatch period for each task
- Indicates task order/index

### Event Log
- Scrollable list of all events
- Color-coded by event type
- Shows timestamps, task names, and details

## Event Types

The visualizer recognizes these event types:
- `test_start` - Test begins
- `test_pass` - Test completes successfully
- `task_discovered` - New task found in running state
- `task_init` - Task initialization
- `task_dispatch` - Task dispatch execution
- `dispatch_cycle_start` - Beginning of dispatch cycle
- `dispatch_cycle_end` - End of dispatch cycle

## Building for Production

```bash
npm run build
npm run preview
```

This creates an optimized build in the `dist` directory.
