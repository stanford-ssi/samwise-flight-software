if(BUILD_TESTS)
  # For test builds, create library that links against mocks
  add_library(burn_wire_state burn_wire_state.c)

  # Link against test mocks and minimal dependencies
  target_link_libraries(burn_wire_state PUBLIC
    test_mocks
    burn_wire_task
    running_state
  )

  # Set include paths: test_mocks first to shadow real hardware headers
  target_include_directories(burn_wire_state PUBLIC
    "${PROJECT_SOURCE_DIR}/src/test_mocks"
    "${PROJECT_SOURCE_DIR}/src/states/burn_wire"
  )
else()
  # For embedded builds, use real hardware dependencies
  add_library(burn_wire_state burn_wire_state.c)

  # Specify targets to link compiled binaries
  target_link_libraries(burn_wire_state PUBLIC common burn_wire_task running_state slate error hardware_spi burn_wire)

  # Specify directories with relevant header files
  target_include_directories(burn_wire_state PUBLIC
    "${PROJECT_SOURCE_DIR}/src/states/burn_wire"
  )
endif()
