add_library(print_task print_task.c)

# Specify targets to link compiled binaries
target_link_libraries(print_task PUBLIC slate common error hardware_spi)

# Specify directories with relevant header files
target_include_directories(print_task PUBLIC
  "${PROJECT_SOURCE_DIR}/src/tasks/print"
)

add_subdirectory(test)
