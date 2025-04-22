add_library(rfm9x 
    rfm9x.c 
    rfm9x_spi.c
    rfm9x_registers.c
    rfm9x_packet.c
    rfm9x_interrupts.c
    rfm9x_utils.c
)

target_link_libraries(rfm9x PRIVATE 
    common 
    pico_stdlib 
    hardware_spi 
    logger
)

target_include_directories(rfm9x PUBLIC 
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/src/drivers/rfm9x"
)
