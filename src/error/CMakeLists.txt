add_library(error error.c)
target_link_libraries(error PUBLIC common pico_stdlib)

target_include_directories(error PUBLIC
  "${PROJECT_SOURCE_DIR}/src/error"
)
