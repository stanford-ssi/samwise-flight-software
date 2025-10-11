if(BUILD_TESTS)
  # For test builds, create library that links against mocks
  add_library(payload_task payload_task.c)

  # Link against test mocks
  target_link_libraries(payload_task PUBLIC test_mocks)

  # Set include paths: test_mocks first to shadow real hardware headers
  target_include_directories(payload_task PUBLIC
    "${PROJECT_SOURCE_DIR}/src/test_mocks"
    "${PROJECT_SOURCE_DIR}/src/tasks/payload"
  )
else()
  # For embedded builds, use real hardware dependencies
  add_library(payload_task payload_task.c)

  target_link_libraries(payload_task PUBLIC payload_uart utils slate common logger neopixel)

  target_include_directories(payload_task PUBLIC
      "${PROJECT_SOURCE_DIR}/src/tasks/payload"
  )
endif()
