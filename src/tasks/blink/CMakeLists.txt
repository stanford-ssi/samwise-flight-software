if(BUILD_TESTS)
  # For test builds, create library that links against mocks
  add_library(blink_task blink_task.c)

  # Link against test mocks
  target_link_libraries(blink_task PUBLIC test_mocks)

  # Set include paths: test_mocks first to shadow real hardware headers
  target_include_directories(blink_task PUBLIC
    "${PROJECT_SOURCE_DIR}/src/test_mocks"
    "${PROJECT_SOURCE_DIR}/src/tasks/blink"
  )
else()
  # For embedded builds, use real hardware dependencies
  add_library(blink_task blink_task.c)

  # Specify targets to link compiled binaries
  target_link_libraries(blink_task PUBLIC slate common error pico_stdlib hardware_spi error)

  # Specify directories with relevant header files
  target_include_directories(blink_task PUBLIC
    "${PROJECT_SOURCE_DIR}/src/tasks/blink"
  )
endif()
