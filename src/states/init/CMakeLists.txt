add_library(init_state init_state.c)

# Specify targets to link compiled binaries
target_link_libraries(init_state PUBLIC common slate error hardware_spi running_state
  print_task burn_wire_state rfm9x)

# Specify directories with relevant header files
target_include_directories(init_state PUBLIC
  "${PROJECT_SOURCE_DIR}/src/states/init"
)
