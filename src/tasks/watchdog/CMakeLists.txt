if(BUILD_TESTS)
  # For test builds, create library that links against mocks
  add_library(watchdog_task watchdog_task.c)

  # Link against test mocks
  target_link_libraries(watchdog_task PUBLIC test_mocks)

  # Set include paths: test_mocks first to shadow real hardware headers
  target_include_directories(watchdog_task PUBLIC
    "${PROJECT_SOURCE_DIR}/src/test_mocks"
    "${PROJECT_SOURCE_DIR}/src/tasks/watchdog"
  )
else()
  # For embedded builds, use real hardware dependencies
  add_library(watchdog_task watchdog_task.c)

  # Specify targets to link compiled binaries
  target_link_libraries(watchdog_task PUBLIC common slate error pico_stdlib
    watchdog neopixel)

  # Specify directories with relevant header files
  target_include_directories(watchdog_task PUBLIC
    "${PROJECT_SOURCE_DIR}/src/tasks/watchdog"
  )
endif()
