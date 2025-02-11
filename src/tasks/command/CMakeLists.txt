add_library(command_task command_task.c)

# Specify targets to link compiled binaries
target_link_libraries(command_task PUBLIC slate common error rfm9x hardware_spi)

# Specify directories with relevant header files
target_include_directories(command_task PUBLIC
  "${PROJECT_SOURCE_DIR}/src/tasks/command"
)
