# SAMWISE Flight Computer Code

Flight computer firmware for the SAMWISE 2U cubesat. Designed for the Raspberry
Pi RP2040 and RP2350 microcontrollers.

## Getting Started

See [the onboarding doc](docs/ONBOARDING.md) for development environment setup.

## Design Objectives

Beyond functional parity with the prior CircuitPython-based Sapling firmware,
this project has a few additional goals:
* Reliable OTAs
* Use the [blackboard pattern](https://en.wikipedia.org/wiki/Blackboard_(design_pattern))
* Eliminate heap allocations
* Simple flight/debug configuration
* Clean code