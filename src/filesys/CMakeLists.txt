# Create library (same for both test and embedded builds)
add_library(filesys filesys.c)
# Link libraries
target_link_libraries(filesys PUBLIC
  # tests use test_mocks instead of common/slate/packet/error/pico_stdlib/hardware_spi/rfm9x/neopixel
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,common>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,slate>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,packet>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,error>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,pico_stdlib>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,hardware_spi>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,rfm9x>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,neopixel>
  $<IF:$<BOOL:${BUILD_TESTS}>,test_mocks,mram>
)
# Include directories: tests include test_mocks to shadow real hardware headers
target_include_directories(filesys PUBLIC
  $<$<BOOL:${BUILD_TESTS}>:${PROJECT_SOURCE_DIR}/src/test_mocks>
  "${PROJECT_SOURCE_DIR}/src/filesys"
  "${PROJECT_SOURCE_DIR}/lib/littlefs-cmake"
)

target_link_libraries(filesys PRIVATE littlefs)

# Add tests subdirectory only for test builds
if(BUILD_TESTS)
  add_subdirectory(test)
endif()
