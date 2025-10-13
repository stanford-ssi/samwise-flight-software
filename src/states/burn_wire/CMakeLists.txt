# Create library (same for both test and embedded builds)
add_library(burn_wire_state burn_wire_state.c)

# Link libraries
target_link_libraries(burn_wire_state PUBLIC
  # tests use test_mocks instead of common/slate/error/hardware_spi/burn_wire
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,common>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,slate>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,error>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,hardware_spi>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,burn_wire>
  burn_wire_task
  running_state
)

# Include directories: tests include test_mocks to shadow real hardware headers
target_include_directories(burn_wire_state PUBLIC
  $<$<BOOL:${BUILD_TESTS}>:${PROJECT_SOURCE_DIR}/src/test_mocks>
  "${PROJECT_SOURCE_DIR}/src/states/burn_wire"
)
