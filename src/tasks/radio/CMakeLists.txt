# Create library (same for both test and embedded builds)
add_library(radio_task radio_task.c)

# Link libraries
target_link_libraries(radio_task PUBLIC
  # tests use test_mocks instead of common/slate/packet/error/pico_stdlib/hardware_spi/rfm9x/neopixel
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,common>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,slate>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,packet>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,error>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,pico_stdlib>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,hardware_spi>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,rfm9x>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,neopixel>
)

# Include directories: tests include test_mocks to shadow real hardware headers
target_include_directories(radio_task PUBLIC
  $<$<BOOL:${BUILD_TESTS}>:${PROJECT_SOURCE_DIR}/src/test_mocks>
  "${PROJECT_SOURCE_DIR}/src/tasks/radio"
)

# Add tests subdirectory only for test builds
if(BUILD_TESTS)
  add_subdirectory(test)
endif()
