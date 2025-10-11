if(BUILD_TESTS)
  # For test builds, create library that links against mocks
  add_library(burn_wire_reset_state burn_wire_reset_state.c)

  # Link against test mocks and minimal dependencies
  target_link_libraries(burn_wire_reset_state PUBLIC
    test_mocks
    running_state
  )

  # Set include paths: test_mocks first to shadow real hardware headers
  target_include_directories(burn_wire_reset_state PUBLIC
    "${PROJECT_SOURCE_DIR}/src/test_mocks"
    "${PROJECT_SOURCE_DIR}/src/states/burn_wire_reset"
  )
else()
  # For embedded builds, use real hardware dependencies
  add_library(burn_wire_reset_state burn_wire_reset_state.c)

  # Specify targets to link compiled binaries
  target_link_libraries(burn_wire_reset_state PUBLIC common flash running_state slate error hardware_spi)

  # Specify directories with relevant header files
  target_include_directories(burn_wire_reset_state PUBLIC
    "${PROJECT_SOURCE_DIR}/src/states/burn_wire_reset"
  )
endif()
