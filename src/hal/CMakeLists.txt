# HAL (Hardware Abstraction Layer) library

if(PROFILE STREQUAL "TEST")
    # Test mode - use mock HAL
    add_library(hal hal_mock.c)
    target_compile_definitions(hal PUBLIC TEST_MODE)
    target_include_directories(hal PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
else()
    # Embedded mode - use Pico HAL
    add_library(hal hal_pico.c)
    target_link_libraries(hal 
        pico_stdlib 
        hardware_spi 
        hardware_i2c 
        hardware_uart 
        hardware_pwm 
        hardware_gpio
        hardware_flash
        hardware_sync
    )
    target_include_directories(hal PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
endif()

# Common header file for both modes
target_sources(hal INTERFACE hal_interface.h)

# Add test subdirectory
add_subdirectory(test)