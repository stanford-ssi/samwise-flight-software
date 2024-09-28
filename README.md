# SAMWISE Flight Computer Code

For now intended to be run on a Raspberry Pi Pico (RP2040 based) as a dev board.

## IDE Setup
We strongly recommend using VSCode in combination with the pi pico extension for development.
See the pi pico getting started guide for how to get setup.

## Getting Started
Referecnce `main.c` for the program entry point.

## Core Design Principles
* The ability to perform reliable OTAs, and maintain two independent copies of software is a key requirement
* The codebase will use the blackboard pattern - different components will edit one central state object.
* No heap allocation after init time - buffers must be statically sized and allocated at init time.
* Flight vs non-flight builds are distinguished by a single symbol - to configure a flight build, define the `FLIGHT` symbol in `macros.h`. Other parts of the codebase that need different behavior in test and flight builds should check this symbol (`#ifdef FLIGHT` or `if (IS_FLIGHT) ...`).
* Where possible, try to explicitly check for errors (to avoid weird undefined C behavior).

## Style Guidelines
* Comments abve functions in the `.c` files, minimal `.h` files
* Doxygen comments for functions

(feel free to fight me on the clang format settings)

Started by Niklas Vainio, 2024/08/23
Stanford SSI Satellites 2024