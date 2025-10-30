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
