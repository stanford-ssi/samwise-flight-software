if(PROFILE STREQUAL "TEST")
    # HAL test executable
    add_executable(hal_test hal_test.c)
    target_link_libraries(hal_test hal)
    target_include_directories(hal_test PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../"
        "${PROJECT_SOURCE_DIR}/src/hal"
    )
    
    # Add test
    enable_testing()
    add_test(NAME HALTest COMMAND hal_test)
endif()