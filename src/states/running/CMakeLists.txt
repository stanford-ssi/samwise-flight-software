# Create library (same for both test and embedded builds)
add_library(running_state running_state.c)

# Link libraries
target_link_libraries(running_state PUBLIC
  # tests use test_mocks instead of common/slate
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,common>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,slate>
  adcs_task
  beacon_task
  blink_task
  command_task
  diagnostics_task
  payload_task
  print_task
  radio_task
  telemetry_task
  watchdog_task
  transmit_task
)

# Include directories: tests include test_mocks to shadow real hardware headers
target_include_directories(running_state PUBLIC
  $<$<BOOL:${BUILD_TESTS}>:${PROJECT_SOURCE_DIR}/src/test_mocks>
  "${PROJECT_SOURCE_DIR}/src/states/running"
)

# Add tests subdirectory only for test builds
if(BUILD_TESTS)
  add_subdirectory(test)
endif()
