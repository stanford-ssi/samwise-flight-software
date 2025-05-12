add_library(logger logger.c)

target_link_libraries(logger PRIVATE pico_stdlib pico_stdio)

pico_enable_stdio_usb(logger 1)

target_include_directories(logger PUBLIC
  "${PROJECT_SOURCE_DIR}/src/drivers/logger"
)
