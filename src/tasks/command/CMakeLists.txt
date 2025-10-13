# Create library (same for both test and embedded builds)
add_library(command_task command_task.c command_parser.c)

# Link libraries
target_link_libraries(command_task PUBLIC
  # tests use test_mocks instead of slate/common/error/rfm9x/payload_uart/hardware_spi/neopixel
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,slate>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,common>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,error>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,rfm9x>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,payload_uart>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,hardware_spi>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,neopixel>
)

# Include directories: tests include test_mocks to shadow real hardware headers
target_include_directories(command_task PUBLIC
  $<$<BOOL:${BUILD_TESTS}>:${PROJECT_SOURCE_DIR}/src/test_mocks>
  "${PROJECT_SOURCE_DIR}/src/tasks/command"
)
