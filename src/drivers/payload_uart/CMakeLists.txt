add_library(payload_uart payload_uart.c)

target_link_libraries(payload_uart PRIVATE utils common error pico_stdlib slate)

target_include_directories(payload_uart PUBLIC 
    "${PROJECT_SOURCE_DIR}/src/drivers/payload_uart"
)
