add_library(running_state running_state.c)

# Specify targets to link compiled binaries
target_link_libraries(running_state PUBLIC
  common
  slate
  print_task
  blink_task
  radio_task
  command_task
  beacon_task
  watchdog_task
  telemetry_task
  diagnostics_task
)

# Specify directories with relevant header files
target_include_directories(running_state PUBLIC
  "${PROJECT_SOURCE_DIR}/src/states/running"
)
