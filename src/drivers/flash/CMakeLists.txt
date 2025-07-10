add_library(flash flash.c)

if(PROFILE STREQUAL "TEST")
    target_compile_definitions(flash PUBLIC TEST_MODE)
    target_link_libraries(flash PRIVATE common logger hal)
else()
    target_link_libraries(flash PRIVATE common pico_stdlib hardware_flash
      hardware_sync logger hal)
endif()

target_include_directories(flash PUBLIC 
  "${PROJECT_SOURCE_DIR}/src/drivers/flash"
)

# Add tests if in TEST mode
if(PROFILE STREQUAL "TEST")
    add_subdirectory(test)
endif()
