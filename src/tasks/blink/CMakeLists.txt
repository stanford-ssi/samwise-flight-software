# Create library (same for both test and embedded builds)
add_library(blink_task blink_task.c)

# Link libraries
target_link_libraries(blink_task PUBLIC
  # tests use test_mocks instead of slate/common/error/pico_stdlib/hardware_spi
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,slate>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,common>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,error>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,pico_stdlib>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,hardware_spi>
)

# Include directories: tests include test_mocks to shadow real hardware headers
target_include_directories(blink_task PUBLIC
  $<$<BOOL:${BUILD_TESTS}>:${PROJECT_SOURCE_DIR}/src/test_mocks>
  "${PROJECT_SOURCE_DIR}/src/tasks/blink"
)
