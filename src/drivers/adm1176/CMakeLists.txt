add_library(adm1176 adm1176.c)

target_link_libraries(adm1176 PUBLIC
    pico_stdlib
    hardware_i2c
)

target_include_directories(adm1176 PUBLIC
    "${PROJECT_SOURCE_DIR}/src"
    "${PROJECT_SOURCE_DIR}/src/drivers/adm1176"
) 