add_library(common INTERFACE)

if(PROFILE STREQUAL "TEST")
    # Test mode - no hardware dependencies
    target_compile_definitions(common INTERFACE TEST_MODE)
else()
    # Embedded mode - link hardware libraries
    target_link_libraries(common INTERFACE slate error hardware_pwm)
endif()

target_include_directories(common INTERFACE 
  "${PROJECT_SOURCE_DIR}/src/common"
)
