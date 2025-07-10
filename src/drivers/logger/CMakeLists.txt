add_library(logger logger.c)

if(PROFILE STREQUAL "TEST")
    # Test build - no Pico SDK
    target_compile_definitions(logger PUBLIC TEST_MODE)
else()
    # Embedded build - link Pico SDK
    target_link_libraries(logger PRIVATE pico_stdlib pico_stdio)
    pico_enable_stdio_usb(logger 1)
endif()

target_include_directories(logger PUBLIC
  "${PROJECT_SOURCE_DIR}/src/drivers/logger"
)
