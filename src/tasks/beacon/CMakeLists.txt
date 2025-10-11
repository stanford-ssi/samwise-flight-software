if(BUILD_TESTS)
  # For test builds, create library that links against mocks
  add_library(beacon_task beacon_task.c)

  # Link against test mocks
  target_link_libraries(beacon_task PUBLIC test_mocks)

  # Set include paths: test_mocks first to shadow real hardware headers
  target_include_directories(beacon_task PUBLIC
    "${PROJECT_SOURCE_DIR}/src/test_mocks"
    "${PROJECT_SOURCE_DIR}/src/tasks/beacon"
  )
else()
  # For embedded builds, use real hardware dependencies
  add_library(beacon_task beacon_task.c)

  # Specify targets to link compiled binaries
  target_link_libraries(beacon_task PUBLIC common slate packet error pico_stdlib hardware_spi rfm9x neopixel)

  # Specify directories with relevant header files
  target_include_directories(beacon_task PUBLIC
    "${PROJECT_SOURCE_DIR}/src/tasks/beacon"
  )
endif()
