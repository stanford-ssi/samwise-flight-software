add_compile_definitions(${SAMWISE_DEFINES} COMMIT_HASH="${COMMIT_HASH}")

add_subdirectory(common)
add_subdirectory(drivers)
add_subdirectory(error)
add_subdirectory(init)
add_subdirectory(packet)
add_subdirectory(scheduler)
add_subdirectory(slate)
add_subdirectory(states)
add_subdirectory(tasks)
add_subdirectory(utils)

add_executable(samwise main.c)

pico_set_program_name(samwise samwise)
pico_set_program_version(samwise "0.1")

# pico_enable_stdio_uart(samwise 0)
pico_enable_stdio_usb(samwise 1)

# Pico SDK
target_link_libraries(samwise PRIVATE hardware_spi hardware_i2c
  pico_unique_id pico_stdlib)

# Samwise
target_link_libraries(samwise PRIVATE scheduler error rfm9x slate init logger
  flash neopixel)

pico_add_extra_outputs(samwise)
