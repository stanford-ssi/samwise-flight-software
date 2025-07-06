add_library(burn_wire_task burn_wire_task.c)

# Specify targets to link compiled binaries
target_link_libraries(burn_wire_task PUBLIC slate common logger burn_wire error)

# Specify directories with relevant header files
target_include_directories(burn_wire_task PUBLIC
  "${PROJECT_SOURCE_DIR}/src/tasks/burn_wire"
)
