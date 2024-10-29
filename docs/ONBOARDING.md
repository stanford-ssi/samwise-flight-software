# Sat Software Onboarding

Welcome to software! We put code **in space**!!

# Part -1: History

First, some context.

Long ago, SSI designed a CubeSat "framework" called the PyCubed. It was great,
because it was a "commercial off-the-shelf" satellite, meaning we could build it
quickly and reliably.

While much of the original design had changed up until this year, one design
decision stuck with us: CircuitPython for our flightcode.

Over time, the benefits of Python as a language began to pale in comparison to
its costs: implicit heap allocations caused random (catastrophic) errors, lack
of proper interrupt support meant lots of polling, lack of explicit static
memory allocation made the aforementioned heap allocations worse, etc.

This year, we decided to finally pivot and ***rewrite all flight
software in C***.

We *also* switched microcontrollers from the SAMD51 (which was mostly hidden
from us by CircuitPython) to the lovely RP2350 (A.K.A. the Raspberry Pi Pico 2).

What does that mean for ***you***? Lots of opportunity to build on what you
learned in CS 106A, 106B, 107, 107E, etc. etc. and build some **real satellite
flight code**. Not only is this a *great* learning experience, but it's
**great** on a resume! SSI software alumni have gone on to NASA, SpaceX, and
grad school ;)

Today we're going to be building a blinking LED. That sounds trivial, but it's a
good way to get all the tools you need set up.

# Part 0: Install

### MacOS

You will need to use the terminal to install some programs. If you are not
familiar with a terminal, I recommend first installing [Visual Studio
Code](https://code.visualstudio.com/). Then, you should use the VS Code
integrated terminal (tutorial
[here](https://code.visualstudio.com/docs/terminal/basics)). Ubuntu has a neat
[tutorial](https://ubuntu.com/tutorials/command-line-for-beginners#4-creating-folders-and-files) on terminal usage - ignore the Ubuntu specific stuff.

Install Homebrew if you don't already have it:
```
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

After installin Homebrew, close and re-open your terminal.

Install CMake & compiler toolchain:
```
brew install cmake
brew install --cask gcc-arm-embedded
```

Again, after installing both, close and re-open your terminal.

### Linux

For [ubuntu/linux](https://askubuntu.com/questions/1243252/how-to-install-arm-none-eabi-gdb-on-ubuntu-20-04-lts-focal-fossa), ARM recently
changed their method for distributing the tool change. Now you
must manually install. As of this lab, the following works:

        wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2

        sudo tar xjf gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2 -C /usr/opt/

We want to get these binaries on our `$PATH` so we don't have to type the
full path to them every time. There's a fast and messy option, or a slower
and cleaner option.

The fast and messy option is to add symlinks to these in your system `bin`
folder:

        sudo ln -s /usr/opt/gcc-arm-none-eabi-10.3-2021.10/bin/* /usr/bin/

The cleaner option is to add `/usr/opt/gcc-arm-none-eabi-10.3-2021.10/bin` to
your `$PATH` variable in your shell configuration file (e.g., `.zshrc` or
`.bashrc`), save it, and `source` the configuration. When you run:

        arm-none-eabi-gcc
        arm-none-eabi-ar
        arm-none-eabi-objdump

You should not get a "Command not found" error.

If gcc can't find header files, try:

       sudo apt-get install libnewlib-arm-none-eabi

### Windows

Unfortunately, I have not gotten to fully supporting Windows. The best option is
to install the [compiler toolchain](https://developer.arm.com/downloads/-/gnu-rm) & [CMake](https://cmake.org/download/) according to their respective websites.

# Part 1: Blinking LED

All microcontrollers come with some pins you can turn on and off, communicate
over, etc. These pins are called *general purpose input/output pins* or GPIO
pins. You can configure them from software.

Generally, these are the steps:
- Initialize the pin
- Set the direction (input or output) of the pin
- Write (if it's output) or read (if it's input) from the pin

Some GPIO pins are connected to physical wires on the board, whereas others are
routed to built-in components. For this example, we are going to use pin 25,
which is routed to an LED on the board. This magic number is provided with a
convenient name in the SDK: `PICO_DEFAULT_LED_PIN`.

Microcontrollers also come with some timing facilities. For now, all we need to
use is the sleeping methods, which are exactly like the ones from Python.

## Git

You should create a clone of [this tepository](https://github.com/shetaye/ssi-onboarding-24/tree/main) using VSCode
(or a git client of your choice). If you are using VSCode, they provide a
[tutorial](https://code.visualstudio.com/docs/sourcecontrol/intro-to-git) on how
to do this.

The rest of this guide takes place from within this folder/repository.

To download the SDK, you need to run
```
git submodule update --init
```
from within the cloned repository (e.g. `ssi-onboarding-24`). You will need to
do this once per clone. You should see a bunch of files appear in `pico-sdk/`.

You also need to run the same command within the `pico-sdk` directory:

```
cd pico-sdk
git submodule update --init
```

## Code

Put this in `main.c`
```c
#include "pico/stdlib.h"

int main() {
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    while (1) {
        gpio_put(PICO_DEFAULT_LED_PIN, 0);
        sleep_ms(250);
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
        sleep_ms(1000);
    }
}
```

## Building

We use **CMake** to build our project. It is like Makefiles for Makefiles.
Honestly, it isn't pretty, but it works quite well! CMake needs to be aware of
all your source files. Each source file is tied to a single *executable* that
CMake will build.

For this example, all you need to do is go to `CMakeLists.txt` and replace `#
FIXME #` with the name of the c file you put all the prior code into.

In a terminal (e.g. the integrated VSCode terminal), make a new directory called
`build` (at the same level as `CMakeLists.txt` and `main.c`).

Your directory structure should look like:

```
ssi-onboarding-24/
    build/
    CMakeLists.txt
    main.c
    README.md
    <whatever other files are in here>
```

Because of the way we have configured CMake, there is one weird step. Run these
commands:

```
mkdir -p ~/.pico-sdk/cmake
touch ~/.pico-sdk/cmake/pico-vscode.cmake
```

Now, enter `build`, then initialize CMake:

```
cd build/
cmake ..
```

Re-run `cmake ..`, and keep repeating this process while pop-ups happen.

Once `cmake ..` has finished, run the following from within `build`:
```
make
```

You should now have a file at
```
ssi-onboarding-24/
    build/
        onboarding.uf2
```

You will repeat the `cd build/`, `cmake ..`, and `make` steps every time you want to make changes

## Uploading

Unplug your pi, hold the button on your pi, plug it in, then move
`build/onboarding.uf2` into the drive that shows up. The light should start to
blink!

# Part 1.5: Github

Now we're going to write code within the actual SSI codebase.

## Git

We will get each of you set up on the GitHub. If you don't already have an
account, make one [here](https://github.com).

If you have no or little experience with Git, I recommend installing [Github
Desktop](https://desktop.github.com/download/). We will be running a special
onboarding session for Git later on.

## Repository

Clone *this* repository via whatever Git client you want

# Part 2: Scheduler

The flight software is organized into a state machine. The satellite can be in
any of several states e.g. "running," "low power," etc.

Each state is associated with tasks that are run regularily (e.g. every half a
second) while the satellite is in that state e.g. the running state might be
associated with a "beacon" task.

If a task is associated with the active state, its `task_dispatch` function is
called at most every `dispatch_period_ms` milliseconds. Additionally, its
`task_init` function is called as part of satellite boot.

You will also see references to a "slate." The slate is a huge block of
statically allocated memory. 

For this part, you are going to write your own task.

## Code Navigation

The scheduler code is located in `scheduler/`. It isn't necessary for this part,
but it might be interesting to you.

The state machine is located in `state_machine/`, with tasks in
`state_machine/tasks` and states in `state_machine/states`. There is a README in
`state_machine` that explains how to add new tasks and states.

## Your task

Make it do whatever you want!