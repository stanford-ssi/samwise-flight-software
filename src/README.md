# Building executable

## Pre-requisite

Download and install gcc-arm-none-eabi compiler: https://github.com/stanford-ssi/samwise-flight-software/blob/main/docs/ONBOARDING.md#linux

## Building from source

1. Update submodule files with `git submodule update --init --recursive`
2. Create build directory `mkdir build` and navigate into it
3. Cmake with a particular platform (e.g. PICO, PICUBED-DEBUG) `cmake -DPROFILE=PICUBED-DEBUG`
4. Make and compile binary `make -j8`

# Code structure

The layout of our flight software is loosely organized into 3 categories:
- Hardware Application Layer (HAL) contained within the `/drivers` subdirectory
- Common headers and shared data structure contained within the `/common` and `/slate` subdirectories respectively
- Finite-State-Machine (FSM) main event loop which is defined by:
    - `/scheduler`: the execution main loop that implements state transitions
    - `/states`: definition of states the satellite can be in
    - `/tasks`: implementation of various isolated tasks that can be performed in any state
    - Initial entrypoint state is defined statically as [`init_state`](https://github.com/stanford-ssi/samwise-flight-software/blob/3203a54866b889707dfc175f2ac8ba17fd0a4b93/src/scheduler/scheduler.c#L20)

```
SAMWISE-FLIGHT-SOFTWARE
├── src
│   ├── common 		        # common headers
│   ├── drivers 		# hardware-specific implementations
│   │   ├── burn_wire
│   │   ├── flash
│   │   ├── logger
│   │   ├── onboard_led
│   │   ├── rfm9x
│   │   ├── watchdog
│   │   └── CMakeLists.txt
│   ├── error			# hardware specific error behavior
│   ├── init			# system initialization
│   ├── scheduler		# main event loop scheduler
│   │   ├── CMakeLists.txt
│   │   ├── scheduler.c
│   │   ├── scheduler.h
│   │   ├── state_machine.h
│   │   └── states.h
│   ├── slate			# global shared data structure
│   │   ├── CMakeLists.txt
│   │   ├── packet.h
│   │   ├── slate.c
│   │   └── slate.h
│   ├── states			# states in FSM
│   │   ├── bringup
│   │   ├── init
│   │   ├── running
│   │   └── CMakeLists.txt
│   └── tasks			# standalone executable tasks
│       ├── beacon
│       ├── blink
│       ├── command
│       ├── diagnostics
│       ├── print
│       ├── radio
│       ├── watchdog
│       └── CMakeLists.txt
├── main.c			# main entry point into software
└── CMakeLists.txt
```

## Contributing

Please view open issues and ongoing PRs before contributing. To contribute a new feature, please:

1. Raise a high-level issue describing what the problem is, what is to be implemented and why. Also include a list of milestones and what success looks like after implementing this feature.
2. Raise additional issues setting their parent issue to the above high-level issue for specific implementation steps. At the minimum, one issue should be raised for each milestone. Issues can be nested under milestones and more granular sub-issues should be raised for each aspect of your contribution (e.g. hardware integration testing, test runs, writing documentation).
3. Assign high-level issue to a core contributor ([@devyaoyh](https://github.com/devyaoyh), [@quackwifhat](https://github.com/quackwifhat), [@niklas-vainio](https://github.com/niklas-vainio), [@shetaye](https://github.com/shetaye)) for triaging before starting.
