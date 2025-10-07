if(BUILD_TESTS)
  # For test builds, create library that links against mocks
  add_library(init_state init_state.c)

  # Link against test mocks and minimal dependencies
  target_link_libraries(init_state PUBLIC
    test_mocks
    running_state
    print_task
  )

  # Set include paths: test_mocks first to shadow real hardware headers
  target_include_directories(init_state PUBLIC
    "${PROJECT_SOURCE_DIR}/src/test_mocks"
    "${PROJECT_SOURCE_DIR}/src/states/init"
  )
else()
  # For embedded builds, use real hardware dependencies
  add_library(init_state init_state.c)

  # Specify targets to link compiled binaries
  target_link_libraries(init_state PUBLIC common slate error hardware_spi running_state
    print_task rfm9x)

  # Specify directories with relevant header files
  target_include_directories(init_state PUBLIC
    "${PROJECT_SOURCE_DIR}/src/states/init"
  )
endif()
