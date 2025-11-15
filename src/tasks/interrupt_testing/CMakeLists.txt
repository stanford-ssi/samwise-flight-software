add_library(interrupt_task interrupt_task.c)

# Link libraries
target_link_libraries(interrupt_task PUBLIC
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
)

# Specify targets to link compiled binaries
target_link_libraries(interrupt_task PUBLIC slate common logger neopixel)

# Specify directories with relevant header files
target_include_directories(interrupt_task PUBLIC
  "${PROJECT_SOURCE_DIR}/src/tasks/interrupt_testing"
  $<$<BOOL:${BUILD_TESTS}>:${PROJECT_SOURCE_DIR}/src/test_mocks>
)