# Create library (same for both test and embedded builds)
add_library(bringup_state bringup_state.c)

# Link libraries
target_link_libraries(bringup_state PUBLIC
  # tests use test_mocks instead of common/slate
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,common>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,slate>
  diagnostics_task
  print_task
  watchdog_task
  running_state
  init_state
)

# Include directories: tests include test_mocks to shadow real hardware headers
target_include_directories(bringup_state PUBLIC
  $<$<BOOL:${BUILD_TESTS}>:${PROJECT_SOURCE_DIR}/src/test_mocks>
  "${PROJECT_SOURCE_DIR}/src/states/bringup"
)
