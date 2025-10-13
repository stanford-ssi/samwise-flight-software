# Create library (same for both test and embedded builds)
add_library(payload_task payload_task.c)

# Link libraries
target_link_libraries(payload_task PUBLIC
  # tests use test_mocks instead of payload_uart/slate/common/logger/neopixel
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,payload_uart>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,slate>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,common>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,logger>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,neopixel>
  utils
)

# Include directories: tests include test_mocks to shadow real hardware headers
target_include_directories(payload_task PUBLIC
  $<$<BOOL:${BUILD_TESTS}>:${PROJECT_SOURCE_DIR}/src/test_mocks>
  "${PROJECT_SOURCE_DIR}/src/tasks/payload"
)
