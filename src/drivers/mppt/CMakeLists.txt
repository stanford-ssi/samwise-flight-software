add_library(mppt mppt.c)

target_link_libraries(mppt PUBLIC
    pico_stdlib
    hardware_i2c
    error
    common
)

target_include_directories(mppt PUBLIC
    "${PROJECT_SOURCE_DIR}/src"
    "${PROJECT_SOURCE_DIR}/src/drivers/mppt"
) 
