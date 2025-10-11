# Helper functions for creating Samwise unit tests
#
# This module provides functions to easily create unit tests that work with
# mocked hardware dependencies for local (non-embedded) testing.

# samwise_add_test: Add a unit test executable
#
# Usage:
#   samwise_add_test(
#     NAME test_name
#     SOURCES test_file.c [additional_sources...]
#     LIBRARIES library_to_test [additional_libs...]
#   )
#
# This function:
# - Creates a test executable
# - Links against test_mocks for hardware abstraction
# - Sets up include paths so mocks shadow real hardware headers
# - Registers the test with CTest
#
# Example:
#   samwise_add_test(
#     NAME print_task_test
#     SOURCES print_test.c
#     LIBRARIES print_task
#   )
function(samwise_add_test)
    # Parse arguments
    set(options "")
    set(oneValueArgs NAME)
    set(multiValueArgs SOURCES LIBRARIES)
    cmake_parse_arguments(TEST "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Validate required arguments
    if(NOT TEST_NAME)
        message(FATAL_ERROR "samwise_add_test: NAME is required")
    endif()
    if(NOT TEST_SOURCES)
        message(FATAL_ERROR "samwise_add_test: SOURCES is required")
    endif()

    # Create the test executable
    add_executable(${TEST_NAME} ${TEST_SOURCES})

    # Link against test mocks and specified libraries
    target_link_libraries(${TEST_NAME} PRIVATE
        test_mocks
        ${TEST_LIBRARIES}
    )

    # Set up include directories
    # Key: test_mocks comes FIRST, so mock headers shadow real hardware headers
    target_include_directories(${TEST_NAME} PRIVATE
        ${PROJECT_SOURCE_DIR}/src/test_mocks
        ${CMAKE_CURRENT_SOURCE_DIR}
    )

    # Register with CTest
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})

    message(STATUS "Added test: ${TEST_NAME}")
endfunction()
