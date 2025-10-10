# Test mocks library for unit testing
# These mocks replace embedded hardware dependencies with test-friendly implementations
add_library(test_mocks
    logger.c
    error.c
    neopixel.c
    driver_stubs.c
    pico/time.c
)

# Make mock headers available - they will shadow real headers when this
# directory is added first in the include path
target_include_directories(test_mocks PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    "${PROJECT_SOURCE_DIR}/src"
    "${PROJECT_SOURCE_DIR}/src/common"
    "${PROJECT_SOURCE_DIR}/src/packet"
    "${PROJECT_SOURCE_DIR}/src/scheduler"
    "${PROJECT_SOURCE_DIR}/src/tasks/command"
    "${PROJECT_SOURCE_DIR}/src/drivers/rfm9x"
    "${PROJECT_SOURCE_DIR}/src/drivers/payload_uart"
    "${PROJECT_SOURCE_DIR}/src/drivers/onboard_led"
    "${PROJECT_SOURCE_DIR}/src/drivers/adcs"
    "${PROJECT_SOURCE_DIR}/src/drivers/watchdog"
    "${PROJECT_SOURCE_DIR}/src/drivers/burn_wire"
    "${PROJECT_SOURCE_DIR}/src/drivers/flash"
    "${PROJECT_SOURCE_DIR}/src/drivers/adm1176"
    "${PROJECT_SOURCE_DIR}/src/drivers/mppt"
    "${PROJECT_SOURCE_DIR}/src/drivers/device_status"
    "${PROJECT_SOURCE_DIR}/src/utils"
    "${PROJECT_SOURCE_DIR}/lib/tinycrypt/lib/include"
)
