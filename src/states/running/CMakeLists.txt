if(BUILD_TESTS)
  # For test builds, create library that links against mocks
  add_library(running_state running_state.c)

  # Link against test mocks and task libraries
  target_link_libraries(running_state PUBLIC
    test_mocks
    adcs_task
    beacon_task
    blink_task
    command_task
    diagnostics_task
    payload_task
    print_task
    radio_task
    telemetry_task
    watchdog_task
  )

  # Set include paths: test_mocks first to shadow real hardware headers
  target_include_directories(running_state PUBLIC
    "${PROJECT_SOURCE_DIR}/src/test_mocks"
    "${PROJECT_SOURCE_DIR}/src/states/running"
  )

  # Add tests
  add_subdirectory(test)
else()
  # For embedded builds, use real hardware dependencies
  add_library(running_state running_state.c)

  # Specify targets to link compiled binaries
  target_link_libraries(running_state PUBLIC
    adcs_task
    beacon_task
    blink_task
    command_task
    common
    diagnostics_task
    payload_task
    print_task
    radio_task
    slate
    telemetry_task
    watchdog_task
  )

  # Specify directories with relevant header files
  target_include_directories(running_state PUBLIC
    "${PROJECT_SOURCE_DIR}/src/states/running"
  )
endif()
