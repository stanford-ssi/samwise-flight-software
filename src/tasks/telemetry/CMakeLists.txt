# Create library (same for both test and embedded builds)
add_library(telemetry_task telemetry_task.c)

# Link libraries
target_link_libraries(telemetry_task PUBLIC
  # tests use test_mocks instead of adm1176/common/device_status/error/hardware_i2c/logger/mppt/neopixel/pico_stdlib/rfm9x/slate
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,adm1176>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,common>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,device_status>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,error>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,hardware_i2c>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,logger>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,mppt>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,neopixel>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,pico_stdlib>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,rfm9x>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,slate>
)

# Include directories: tests include test_mocks to shadow real hardware headers
target_include_directories(telemetry_task PUBLIC
  $<$<BOOL:${BUILD_TESTS}>:${PROJECT_SOURCE_DIR}/src/test_mocks>
  "${PROJECT_SOURCE_DIR}/src/tasks/telemetry"
)
