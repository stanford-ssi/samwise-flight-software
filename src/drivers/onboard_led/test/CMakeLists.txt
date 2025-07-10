if(PROFILE STREQUAL "TEST")
    add_executable(onboard_led_test onboard_led_test.c)
    
    target_compile_definitions(onboard_led_test PRIVATE TEST_MODE)
    target_link_libraries(onboard_led_test PRIVATE 
        hal 
        onboard_led 
    )
    
    target_include_directories(onboard_led_test PRIVATE
        "${PROJECT_SOURCE_DIR}/src/drivers/onboard_led/test"
        "${PROJECT_SOURCE_DIR}/src/hal"
    )
endif()