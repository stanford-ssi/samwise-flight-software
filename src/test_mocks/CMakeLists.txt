# Test mocks library for unit testing
# These mocks replace embedded hardware dependencies with test-friendly implementations
add_library(test_mocks
    logger.c
    error.c
    neopixel.c
)

# Make mock headers available - they will shadow real headers when this
# directory is added first in the include path
target_include_directories(test_mocks PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
