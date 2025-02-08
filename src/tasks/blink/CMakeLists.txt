add_library(blink_task blink_task.c)

# Specify targets to link compiled binaries
target_link_libraries(blink_task PUBLIC slate common error pico_stdlib hardware_spi error)

# Specify directories with relevant header files
target_include_directories(blink_task PUBLIC
  "${PROJECT_SOURCE_DIR}/src/tasks/blink"
)
