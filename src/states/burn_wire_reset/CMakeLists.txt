add_library(burn_wire_reset_state burn_wire_reset_state.c)

# Specify targets to link compiled binaries
target_link_libraries(burn_wire_reset_state PUBLIC common flash running_state slate error hardware_spi)

# Specify directories with relevant header files
target_include_directories(burn_wire_reset_state PUBLIC
  "${PROJECT_SOURCE_DIR}/src/states/burn_wire_reset"
)
