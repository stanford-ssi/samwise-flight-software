add_library(burn_wire_state burn_wire_state.c)

# Specify targets to link compiled binaries
target_link_libraries(burn_wire_state PUBLIC common burn_wire_task running_state slate error hardware_spi burn_wire)

# Specify directories with relevant header files
target_include_directories(burn_wire_state PUBLIC
  "${PROJECT_SOURCE_DIR}/src/states/burn_wire"
)
