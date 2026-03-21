# SAMWISE Flight Computer Code

Flight computer firmware for the SAMWISE 2U cubesat. Designed for the Raspberry
Pi RP2040 and RP2350 microcontrollers.

![PXL_20251105_063120461](https://github.com/user-attachments/assets/f1ea9d1d-33db-42ee-8892-54c7ed03953b)

## Design Objectives

Beyond functional parity with the prior CircuitPython-based Sapling firmware,
this project has a few additional goals:
* Reliable OTAs
* Use the [blackboard pattern](https://en.wikipedia.org/wiki/Blackboard_(design_pattern))
* Eliminate heap allocations
* Simple flight/debug configuration
* Clean code

## Getting Started

### Pre-requisites

1. Install bazel by following instructions for your platform here:

2. Pull necessary submodules using:

```
git submodule update --init --recursive
```

> Historically, see [the onboarding doc](docs/ONBOARDING.md) for development environment setup (pre-bazel).

## Testing
Running tests can be done with `source run_tests.sh`. See `TESTING.md` for more information.


## Building
To build the code in this repo, run `bazel build :samwise --config=picubed-debug`, or any other config option.

The following configuration options are available:
* `pico`: pico exectuable
* `picubed-debug`: picubed executable, for debugging
* `picubed-flight`: picubed executable, for flight
* `picubed-bringup`: picubed executable, for bringing up the board
(these can be configured in `.bazelrc`)

### Build Archives
The **C Build** github action automatically builds RP2350 archives on pushes to pull requests into main.
