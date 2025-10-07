if(BUILD_TESTS)
  # For test builds, create a minimal mock library
  add_library(print_task print_task.c)

  # Link against test mocks
  target_link_libraries(print_task PUBLIC test_mocks)

  # Include test directory first to override headers with mocks
  target_include_directories(print_task PUBLIC
    "${PROJECT_SOURCE_DIR}/src/tasks/print/test"
    "${PROJECT_SOURCE_DIR}/src/test_mocks"
    "${PROJECT_SOURCE_DIR}/src/tasks/print"
  )

  # Add subdirectory for tests
  add_subdirectory(test)
else()
  # For normal builds, build with all dependencies
  add_library(print_task print_task.c)

  # Specify targets to link compiled binaries
  target_link_libraries(print_task PUBLIC slate common logger neopixel)

  # Specify directories with relevant header files
  target_include_directories(print_task PUBLIC
    "${PROJECT_SOURCE_DIR}/src/tasks/print"
  )
endif()
