# Create library (same for both test and embedded builds)
add_library(ftp_task ftp_task.c)

# Link libraries
target_link_libraries(ftp_task PUBLIC
  # tests use test_mocks instead of pico_stdlib/hardware_i2c/hardware_spi/pico_unique_id/logger/slate/common/error/rfm9x/adm1176
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,pico_stdlib>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,hardware_i2c>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,hardware_spi>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,pico_unique_id>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,logger>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,slate>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,common>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,error>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,rfm9x>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,adm1176>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,mram>
  filesys
)

# Include directories: tests include test_mocks to shadow real hardware headers
target_include_directories(ftp_task PUBLIC
  $<$<BOOL:${BUILD_TESTS}>:${PROJECT_SOURCE_DIR}/src/test_mocks>
  "${PROJECT_SOURCE_DIR}/src/tasks/ftp"
  "${PROJECT_SOURCE_DIR}/lib/littlefs-cmake"
)

if (BUILD_TESTS)
  add_subdirectory(test)
endif()
