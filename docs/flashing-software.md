# Uploading new Software binaries to PiCubed

## Copying files over using BOOT mode

To put the PiCubed into BOOT mode, you should press and hold down the BOOTSEL button **BEFORE** turning the device on!!!
- This means **before you plug the PiCubed into your computer**
- Also means **you should NOT plug in the battery board to the PiCubed yet**

**!IMPORTANT** CONTINUE TO PRESS AND HOLD DOWN THE BOOTSEL BUTTON EVEN AFTER YOU PLUG THE PICUBED INTO YOUR COMPUTER FOR AT LEAST 2 SECONDS.

After you release the BOOTSEL button, you should see a new drive/folder mounted onto your computer's filesystem (like a USB stick). You might need to grant it permissions to connect at this point (e.g. on MacOS).

See this video (although it's for a Pi Pico 2, we're essentially using the same board): https://www.youtube.com/watch?v=2d21MBF4i6Q&t=46s

Once this is done, as illustrated in the video above, you just need to drag over (copy) the software .uf2 file you want to install into the newly mounted PICO directory. The device should automatically disconnect and reboot itself to start running the new software. At this point, installation is complete and you can disconnect the device from your computer for integration into the Satellite structure.

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
