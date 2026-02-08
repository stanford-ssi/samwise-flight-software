# Create library (same for both test and embedded builds)
#
# In BRINGUP builds the test source files are compiled into the library so
# hardware_test_task can call each test's entry point at init time.
if(NOT BUILD_TESTS)
  add_library(hardware_test_task
    hardware_test_task.c
    test/mram_test.c          # ← add new test .c files here for BRINGUP
  )
else()
  add_library(hardware_test_task hardware_test_task.c)
endif()

# In BRINGUP builds, expose driver headers needed by the test sources.
if(NOT BUILD_TESTS)
  target_include_directories(hardware_test_task PRIVATE
    "${PROJECT_SOURCE_DIR}/src/drivers/mram"   # ← add driver include dirs here
  )
endif()

target_link_libraries(hardware_test_task PUBLIC
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
)

# Link hardware drivers needed by the tests in BRINGUP builds
if(NOT BUILD_TESTS)
  target_link_libraries(hardware_test_task PUBLIC mram)
endif()

# Do NOT link mocks, this should only be run on hardware.
target_include_directories(hardware_test_task PUBLIC
   $<$<BOOL:${BUILD_TESTS}>:${PROJECT_SOURCE_DIR}/src/test_mocks> 
  "${PROJECT_SOURCE_DIR}/src/tasks/hardware_test"
)

if (BUILD_TESTS)
  add_subdirectory(test)
endif()
