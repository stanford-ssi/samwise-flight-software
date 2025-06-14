add_library(error error.c)
target_link_libraries(error PUBLIC common utils pico_stdlib neopixel)

target_include_directories(error PUBLIC
  "${PROJECT_SOURCE_DIR}/src/error"
)
