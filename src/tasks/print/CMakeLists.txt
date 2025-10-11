if(BUILD_TESTS)
  # For test builds, create library that links against mocks
  add_library(print_task print_task.c)

  # Link against test mocks (hardware abstraction for testing)
  target_link_libraries(print_task PUBLIC test_mocks)

  # Set include paths: test_mocks first to shadow real hardware headers
  target_include_directories(print_task PUBLIC
    "${PROJECT_SOURCE_DIR}/src/test_mocks"
    "${PROJECT_SOURCE_DIR}/src/tasks/print"
  )

  # Add tests
  add_subdirectory(test)
else()
  # For embedded builds, use real hardware dependencies
  add_library(print_task print_task.c)
  target_link_libraries(print_task PUBLIC slate common logger neopixel)
  target_include_directories(print_task PUBLIC
    "${PROJECT_SOURCE_DIR}/src/tasks/print"
  )
endif()
