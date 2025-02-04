# SAMWISE Flight Computer Code

Flight computer firmware for the SAMWISE 2U cubesat. Designed for the Raspberry
Pi RP2040 and RP2350 microcontrollers.

## Getting Started

See [the onboarding doc](docs/ONBOARDING.md) for development environment setup. To set some useful configurations for this repo, run `configure.sh`.

## Design Objectives

Beyond functional parity with the prior CircuitPython-based Sapling firmware,
this project has a few additional goals:
* Reliable OTAs
* Use the [blackboard pattern](https://en.wikipedia.org/wiki/Blackboard_(design_pattern))
* Eliminate heap allocations
* Simple flight/debug configuration
* Clean code

## Building
To build the code in this repo, enter the build folder with `cd build` (if you do not have one), and then run `cmake ..` followed by `make -j8`.

The following targets will be built:
* `samwise_pico_debug`: pico exectuable
* `samwise_picubed_debug`: picubed executable, for debugging
* `samwise_picubed_flight`: picubed executable, for flight
* `samwise_picubed_bringup`: picubed executable, for bringing up the board
(these can be configured in `CMakeLists.txt`)

### Build Archives
The **C Build** github action automatically builds RP2040 and RP2350 archives on pushes to pull requests into main.

## Switching build targets
To switch between builds for the RP2040 (Pico 1) and RP2350 (Pico 2 and Pycubed), set the `RP2350` flag to 1 when calling `cmake`. For example, to build for the RP2350, run:
```
cmake .. -DRP2350=1
```

**Note that when switching between the RP2040 and RP2350, it is usually necessary to clear the cmake cache (by running `rm -rf build/*`)**