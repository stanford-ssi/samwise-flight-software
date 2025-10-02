add_library(command_task command_task.c command_parser.c)

# Specify targets to link compiled binaries
target_link_libraries(command_task PUBLIC slate common error rfm9x payload_uart hardware_spi neopixel)

# Specify directories with relevant header files
target_include_directories(command_task PUBLIC
  "${PROJECT_SOURCE_DIR}/src/tasks/command"
)
