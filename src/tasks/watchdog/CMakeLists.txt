# Create library (same for both test and embedded builds)
add_library(watchdog_task watchdog_task.c)

# Link libraries
target_link_libraries(watchdog_task PUBLIC
  # tests use test_mocks instead of common/slate/error/pico_stdlib/watchdog/neopixel
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,common>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,slate>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,error>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,pico_stdlib>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,watchdog>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,neopixel>
)

# Include directories: tests include test_mocks to shadow real hardware headers
target_include_directories(watchdog_task PUBLIC
  $<$<BOOL:${BUILD_TESTS}>:${PROJECT_SOURCE_DIR}/src/test_mocks>
  "${PROJECT_SOURCE_DIR}/src/tasks/watchdog"
)
