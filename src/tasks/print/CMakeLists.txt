# Create library (same for both test and embedded builds)
add_library(print_task print_task.c)

# Link libraries
target_link_libraries(print_task PUBLIC
  # tests use test_mocks instead of slate/common/logger/neopixel
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,slate>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,common>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,logger>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,neopixel>
)

# Include directories: tests include test_mocks to shadow real hardware headers
target_include_directories(print_task PUBLIC
  $<$<BOOL:${BUILD_TESTS}>:${PROJECT_SOURCE_DIR}/src/test_mocks>
  "${PROJECT_SOURCE_DIR}/src/tasks/print"
)

# Add tests (only for test builds)
if(BUILD_TESTS)
  add_subdirectory(test)
endif()
