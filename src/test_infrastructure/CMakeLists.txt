# Shared test infrastructure library
# Provides common utilities for state testing

add_library(test_infrastructure
    test_scheduler_helpers.c
)

target_include_directories(test_infrastructure PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src/test_mocks
    ${PROJECT_SOURCE_DIR}/src/scheduler
    ${PROJECT_SOURCE_DIR}/src/common
)

target_link_libraries(test_infrastructure PUBLIC
    test_mocks
)
