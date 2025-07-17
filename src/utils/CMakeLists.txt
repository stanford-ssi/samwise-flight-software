add_library(utils INTERFACE)

if(PROFILE STREQUAL "TEST")
    # Test mode - no hardware dependencies
    target_compile_definitions(utils INTERFACE TEST_MODE)
    target_link_libraries(utils INTERFACE common hal)
else()
    # Embedded mode - link hardware libraries
    target_link_libraries(utils INTERFACE pico_stdlib slate common watchdog hal)
endif()

target_include_directories(utils INTERFACE 
  "${PROJECT_SOURCE_DIR}/src/utils"
)
