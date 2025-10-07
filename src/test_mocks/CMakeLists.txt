# Test mocks library for unit testing
add_library(test_mocks
    logger_mock.c
    error_mock.c
    neopixel_mock.c
)

target_include_directories(test_mocks PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
