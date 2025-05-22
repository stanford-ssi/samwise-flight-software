add_library(neopixel neopixel.c)

target_link_libraries(neopixel PUBLIC
    pico_stdlib
    hardware_pio
    hardware_pwm
    error
    logger
)

target_include_directories(neopixel PUBLIC
    "${PROJECT_SOURCE_DIR}/src"
    "${PROJECT_SOURCE_DIR}/src/drivers/neopixel"
) 
