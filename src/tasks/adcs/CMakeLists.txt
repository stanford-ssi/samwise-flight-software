# Create library (same for both test and embedded builds)
add_library(adcs_task adcs_task.c)

# Link libraries
target_link_libraries(adcs_task PUBLIC
  # tests use test_mocks instead of slate/common/error/adcs_driver
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,slate>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,common>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,error>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,adcs_driver>
)

# Include directories: tests include test_mocks and common to shadow real hardware headers
target_include_directories(adcs_task PUBLIC
  $<$<BOOL:${BUILD_TESTS}>:${PROJECT_SOURCE_DIR}/src/test_mocks>
  $<$<BOOL:${BUILD_TESTS}>:${PROJECT_SOURCE_DIR}/src/common>
  "${PROJECT_SOURCE_DIR}/src/tasks/adcs"
  "${PROJECT_SOURCE_DIR}/src/drivers/adcs"
)
