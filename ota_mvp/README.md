# Minimal OTA demonstration code

1. Leverage picotool to prepare multiple partitions on the rp2350
2. Write prog_b (hello world) -> compile into .elf to get the program bytes
3. Write prog_a that contains program bytes of prog_b and writes them into the respective regions of flash
4. prog_a should also handle making the sys calls necessary to reboot from newly written prog_b bytes

## Build & Execute

Execute:
```
cmake --build build --target ota
```

From the repo root to only build this demo app (skips samwise build).

## Prepare the partition table

First, we need to "compile" the partition table into a binary format suitable for flashing.

> [!NOTE]
> The RP2350 bootloader looks at the *family id* of the UF2 to determine how to treat it. "Normal" UF2 files have a family ID like `rp2350-arm-ns`, and for those the bootloader tries to place the contents into a partition. If we want to load the partition *itself*, we need to use the `absolute` family ID. For these, the bootloader will always load its contents to the start of flash (or whatever address is specified in the UF2?).

The following command generates an `absolute` UF2 containing the binary format of the partition table:

```
picotool partition create pt.json pt.uf2
```

## Load the partition table

Place your RP2350 into `BOOTSEL` mode, then use

```
picotool load pt.uf2
```

to load the partition table. Now, make sure to reboot so the new one is recognized by the bootloader.

To confirm that it worked, run

```
picotooll partition info
```

which should print out your loaded partition table

## Load 
