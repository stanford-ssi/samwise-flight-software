add_library(radio_task radio_task.c)

# Specify targets to link compiled binaries
target_link_libraries(radio_task PUBLIC common slate error pico_stdlib hardware_spi rfm9x)

# Specify directories with relevant header files
target_include_directories(radio_task PUBLIC
  "${PROJECT_SOURCE_DIR}/src/tasks/radio"
)
