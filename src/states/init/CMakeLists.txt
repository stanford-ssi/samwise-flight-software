# Create library (same for both test and embedded builds)
add_library(init_state init_state.c)

# Link libraries
target_link_libraries(init_state PUBLIC
  # tests use test_mocks instead of common/slate/error/hardware_spi/rfm9x
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,common>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,slate>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,error>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,hardware_spi>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,rfm9x>
  running_state
  print_task
)

# Include directories: tests include test_mocks to shadow real hardware headers
target_include_directories(init_state PUBLIC
  $<$<BOOL:${BUILD_TESTS}>:${PROJECT_SOURCE_DIR}/src/test_mocks>
  "${PROJECT_SOURCE_DIR}/src/states/init"
)
