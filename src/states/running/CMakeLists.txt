add_library(running_state running_state.c)

# Specify targets to link compiled binaries
target_link_libraries(running_state PUBLIC
  adcs_task
  beacon_task
  blink_task
  command_task
  common
  diagnostics_task
  payload_task
  print_task
  radio_task
  slate
  telemetry_task
  watchdog_task
)

# Specify directories with relevant header files
target_include_directories(running_state PUBLIC
  "${PROJECT_SOURCE_DIR}/src/states/running"
)
