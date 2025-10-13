# Create library (same for both test and embedded builds)
add_library(burn_wire_task burn_wire_task.c)

# Link libraries
target_link_libraries(burn_wire_task PUBLIC
  # tests use test_mocks instead of slate/common/logger/burn_wire/error/neopixel
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,slate>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,common>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,logger>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,burn_wire>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,error>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,neopixel>
  utils
)

# Include directories: tests include test_mocks to shadow real hardware headers
target_include_directories(burn_wire_task PUBLIC
  $<$<BOOL:${BUILD_TESTS}>:${PROJECT_SOURCE_DIR}/src/test_mocks>
  "${PROJECT_SOURCE_DIR}/src/tasks/burn_wire"
)
