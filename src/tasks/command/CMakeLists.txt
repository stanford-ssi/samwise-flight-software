if(BUILD_TESTS)
  # For test builds, create library that links against mocks
  add_library(command_task command_task.c command_parser.c)

  # Link against test mocks
  target_link_libraries(command_task PUBLIC test_mocks)

  # Set include paths: test_mocks first to shadow real hardware headers
  target_include_directories(command_task PUBLIC
    "${PROJECT_SOURCE_DIR}/src/test_mocks"
    "${PROJECT_SOURCE_DIR}/src/tasks/command"
  )
else()
  # For embedded builds, use real hardware dependencies
  add_library(command_task command_task.c command_parser.c)

  # Specify targets to link compiled binaries
  target_link_libraries(command_task PUBLIC slate common error rfm9x payload_uart hardware_spi neopixel)

  # Specify directories with relevant header files
  target_include_directories(command_task PUBLIC
    "${PROJECT_SOURCE_DIR}/src/tasks/command"
  )
endif()
