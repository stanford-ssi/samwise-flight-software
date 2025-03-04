add_library(neopixel neopixel.c)

target_link_libraries(neopixel PRIVATE pico_stdlib hardware_clocks hardware_pio)

pico_enable_stdio_usb(neopixel 1)

target_include_directories(neopixel PUBLIC
  "${PROJECT_SOURCE_DIR}/src/drivers/neopixel"
)
