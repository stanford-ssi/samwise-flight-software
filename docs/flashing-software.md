# Uploading new Software binaries to PiCubed

## Using Picotool to install new software (without BOOTSEL button access)

### Pre-requisites

1. Please install picotool here:
    - (For MacOS) https://formulae.brew.sh/formula/picotool
    - (For Linux) https://github.com/raspberrypi/picotool?tab=readme-ov-file#linux--macos
        - One needs to build from source which is more complicated
    - (For Windows) https://github.com/raspberrypi/picotool?tab=readme-ov-file#for-windows-without-mingw
        - Instructions to follow here, looks like even less well supported on windows
2. The device should also already be running a version of the software that's later than release [`dev-v0.0.3`](https://github.com/stanford-ssi/samwise-flight-software/releases/tag/dev-v0.0.3).

### Installation steps
After installing picotool, the two commands from the terminal one would need to use are:

1. Uploading a new software version:

```
picotool load burn_wire_bypass.uf2 -f
```

Note that 'burn_wire_bypass.uf2' should be the relative file path to the software you want to upload from where you're running the command. For example, in the following directory structure where * indicate where one is currently in:

```
Downloads/
├── * <-- You are currently in the 'Downloads' folder
├── software/
│   └── burn_wire_bypass.uf2
└── debug_v2.uf2
```

To load 'debug_v2.uf2', one will do `picotool load debug_v2.uf2 -f`, but to load 'burn_wire_bypass.uf2', one will need to do `picotool load software/burn_wire_bypass.uf2 -f`.

2. Rebooting the device to run the new software

```
picotool reboot
```

This reboots the device into application mode (from installation mode) after you have flashed the new version of the software. This step is **optional** if you simply unplug the device after step 1 (and reboot it manually by plugging in power at a later time).

