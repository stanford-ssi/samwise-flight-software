add_library(error error.c)

if(PROFILE STREQUAL "TEST")
    target_compile_definitions(error PUBLIC TEST_MODE)
    target_link_libraries(error PUBLIC common utils hal)
else()
    target_link_libraries(error PUBLIC common utils pico_stdlib neopixel)
endif()

target_include_directories(error PUBLIC
  "${PROJECT_SOURCE_DIR}/src/error"
)
