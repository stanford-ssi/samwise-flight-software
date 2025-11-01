sudo ~/.pico-sdk/openocd/0.12.0+dev/openocd \
    -s ~/.pico-sdk/openocd/0.12.0+dev/scripts \
    -f interface/cmsis-dap.cfg \
    -f target/rp2350.cfg \
    -c "adapter speed 5000" \
    -c "program ../build/src/samwise.elf verify reset exit"
