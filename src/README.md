# Building executable

## Pre-requisites

1. **Bazel**: Install [Bazelisk](https://github.com/bazelbuild/bazelisk) (recommended) or [Bazel](https://bazel.build/install) directly.
2. **ARM toolchain**: Download and install `gcc-arm-none-eabi`. See the [onboarding doc](https://github.com/stanford-ssi/samwise-flight-software/blob/main/docs/ONBOARDING.md#linux) for platform-specific instructions.
3. **clang-format** (optional, for formatting): `brew install clang-format` on macOS, or `sudo apt install clang-format` on Linux.

## Building from source

Build with a specific profile configuration:

```bash
# PiCubed debug build (most common for development)
bazel build --config=picubed-debug //:samwise

# PiCubed flight build (optimized for deployment)
bazel build --config=picubed-flight //:samwise

# PiCubed bringup build (board bring-up testing)
bazel build --config=picubed-bringup //:samwise

# Pico development build (RP2040)
bazel build --config=pico //:samwise
```

The `.uf2` file is automatically generated alongside the ELF binary for all on-device profiles. Find it at:
```
bazel-bin/samwise.uf2
```

### Running tests

```bash
bazel test //...
```

Tests run on the host platform with mocked hardware drivers. See `bzl/defs.bzl` for the `samwise_test()` macro that handles mock substitution automatically.

## Pre-commit hook

A clang-format pre-commit hook is available to automatically format staged C/H files before each commit. To install it:

```bash
cp scripts/pre-commit-hook.sh .git/hooks/pre-commit
chmod +x .git/hooks/pre-commit
```

Or run `configure.sh` which sets this up for you.

# Code structure

The layout of our flight software is organized into these categories:
- Hardware Application Layer (HAL) in `/drivers`
- Common headers and shared data in `/common` and `/slate`
- Finite-State-Machine (FSM) main event loop:
    - `/scheduler`: the execution main loop, state registry, and state transitions
    - `/states`: definition of states the satellite can be in
    - `/tasks`: implementation of isolated tasks that run within states
    - Initial entrypoint state is `STATE_INIT` (see `scheduler.c`)

### State machine architecture

States are identified by `state_id_t` enums (defined in `scheduler/state_ids.h`) rather than by direct struct pointers. The **state registry** (`scheduler/state_registry.h`) provides O(1) lookup from state ID to `sched_state_t*`. States are registered during `sched_init()` in `scheduler.c`.

Each state defines:
- A list of tasks to dispatch periodically
- A `get_next_state()` function that returns the `state_id_t` to transition to

Build profiles control which states and tasks are active via preprocessor defines (`BRINGUP`, `FLIGHT`, `DEBUG`).

```
src
├── common              # common headers and typedefs
├── drivers             # hardware-specific implementations
│   ├── adcs
│   ├── adm1176
│   ├── burn_wire
│   ├── device_status
│   ├── flash
│   ├── logger
│   ├── mppt
│   ├── mram
│   ├── neopixel
│   ├── onboard_led
│   ├── payload_uart
│   ├── rfm9x
│   └── watchdog
├── error               # hardware-specific error behavior
├── init                # system initialization
├── packet              # packet encoding/decoding
├── scheduler           # main event loop and state machine
│   ├── scheduler.c/h
│   ├── state_machine.h
│   ├── state_ids.h
│   └── state_registry.c/h
├── slate               # global shared data structure
├── states              # states in FSM
│   ├── bringup
│   ├── burn_wire
│   ├── burn_wire_reset
│   ├── init
│   └── running
├── tasks               # standalone executable tasks
│   ├── adcs
│   ├── beacon
│   ├── blink
│   ├── burn_wire
│   ├── command
│   ├── diagnostics
│   ├── payload
│   ├── print
│   ├── radio
│   ├── telemetry
│   └── watchdog
└── utils               # utility functions
```

For guides on adding new states or tasks, see [states/README.md](states/README.md) and [tasks/README.md](tasks/README.md).

## Contributing

Please view open issues and ongoing PRs before contributing. To contribute a new feature, please:

1. Raise a high-level issue describing what the problem is, what is to be implemented and why. Also include a list of milestones and what success looks like after implementing this feature.
2. Raise additional issues setting their parent issue to the above high-level issue for specific implementation steps. At the minimum, one issue should be raised for each milestone. Issues can be nested under milestones and more granular sub-issues should be raised for each aspect of your contribution (e.g. hardware integration testing, test runs, writing documentation).
3. Assign high-level issue to a core contributor ([@devyaoyh](https://github.com/devyaoyh), [@quackwifhat](https://github.com/quackwifhat), [@niklas-vainio](https://github.com/niklas-vainio), [@shetaye](https://github.com/shetaye)) for triaging before starting.
