# Running State Test & Visualizer

## Overview

The running state test has been refactored to use **real scheduler logic** with **time-based execution**, providing accurate simulation of satellite task scheduling behavior.

## Key Changes

### 1. Test Uses Real Scheduler (`src/scheduler/scheduler.c`)
- **No reimplementation**: The test now compiles and uses the actual `scheduler.c` 
- Tests the real task dispatch logic with proper time-based checks
- Verifies tasks execute at their correct periods (100ms, 1000ms, 5000ms)

### 2. Mock Time Infrastructure
- `src/test_mocks/pico/stdlib.h`: Mock time functions that track simulated time
- `src/test_mocks/pico/time.c`: Global `mock_time_us` variable
- Time advances realistically during test execution
- `sleep_ms()` and `sleep_us()` advance the simulation clock

### 3. State Header Mocks
Created minimal mock headers for states not needed in tests:
- `src/test_mocks/init_state.h`
- `src/test_mocks/burn_wire_state.h`
- `src/test_mocks/burn_wire_reset_state.h`
- `src/test_mocks/bringup_state.h`
- `src/test_mocks/states.h` - includes only what's needed

These allow `scheduler.c` to compile without pulling in unnecessary dependencies.

## Running the Test

```bash
cd /code/build
make running_state_test
./src/states/running/test/running_state_test
```

This generates `running_state_viz.json` in the current directory.

## Test Coverage

The test simulates:
- **30 seconds** of satellite operation in time_based_task_dispatching test
- **10 seconds** for dispatch period verification
- Tasks executing at their correct frequencies:
  - **watchdog, radio, command**: every 100ms (fast)
  - **print, blink, telemetry**: every 1000ms (medium)
  - **adcs, beacon**: every 5000ms (slow)

## Using the Visualizer

1. **Setup** (first time only):
   ```bash
   cd /code/web/visualizer
   npm install
   ```

2. **Start the dev server**:
   ```bash
   npm run dev
   ```

3. **Open browser** to http://localhost:3000

4. **Load log file**: Click "Load Log File" and select `running_state_viz.json`

## Visualizer Features

- **Timeline**: Visual representation of task execution over time
- **Task List**: All discovered tasks with their dispatch periods
- **Event Log**: Chronological log of all test events

## Benefits of Real Scheduler Testing

✅ Tests actual production code path  
✅ Catches scheduler bugs that manual dispatch wouldn't  
✅ Verifies time-based task execution logic  
✅ Ensures tasks dispatch at correct periods  
✅ Provides realistic visualization data  
✅ No code duplication between test and production  

## Architecture

```
test_running_state.c  
    ↓ uses
scheduler.c (REAL)
    ↓ calls
running_state tasks
    ↓ execute with
mock_time_us (simulated time)
    ↓ logs to
running_state_viz.json
    ↓ visualized by
web/visualizer (React/TSX)
```
