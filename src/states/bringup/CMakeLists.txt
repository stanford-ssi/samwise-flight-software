if(BUILD_TESTS)
  # For test builds, create library that links against mocks
  add_library(bringup_state bringup_state.c)

  # Link against test mocks and minimal dependencies
  target_link_libraries(bringup_state PUBLIC
    test_mocks
    diagnostics_task
    print_task
    watchdog_task
    running_state
    init_state
  )

  # Set include paths: test_mocks first to shadow real hardware headers
  target_include_directories(bringup_state PUBLIC
    "${PROJECT_SOURCE_DIR}/src/test_mocks"
    "${PROJECT_SOURCE_DIR}/src/states/bringup"
  )
else()
  # For embedded builds, use real hardware dependencies
  add_library(bringup_state bringup_state.c)

  # Specify targets to link compiled binaries
  target_link_libraries(bringup_state PUBLIC
    common
    slate
    diagnostics_task
    print_task
    watchdog_task
    running_state
    init_state
  )

  # Specify directories with relevant header files
  target_include_directories(bringup_state PUBLIC
    "${PROJECT_SOURCE_DIR}/src/states/bringup"
  )
endif()
