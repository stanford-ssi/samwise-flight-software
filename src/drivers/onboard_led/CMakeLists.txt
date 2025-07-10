add_library(onboard_led onboard_led.c)

if(PROFILE STREQUAL "TEST")
    target_compile_definitions(onboard_led PUBLIC TEST_MODE)
endif()

# Always link to hal since we use HAL abstraction
target_link_libraries(onboard_led PRIVATE hal)

# In embedded mode, also link to pico_stdlib
if(NOT PROFILE STREQUAL "TEST")
    target_link_libraries(onboard_led PRIVATE pico_stdlib)
endif()

target_include_directories(onboard_led PUBLIC 
  "${PROJECT_SOURCE_DIR}/src/drivers/onboard_led"
)

# Add tests if in TEST mode
if(PROFILE STREQUAL "TEST")
    add_subdirectory(test)
endif()
