add_executable(ota main.c)

pico_set_program_name(ota ota_demo)
pico_set_program_version(ota "0.1")

# pico_enable_stdio_uart(samwise 0)
pico_enable_stdio_usb(ota 1)

# Pico SDK
target_link_libraries(ota PRIVATE hardware_spi hardware_i2c pico_stdlib)

pico_add_extra_outputs(ota)
