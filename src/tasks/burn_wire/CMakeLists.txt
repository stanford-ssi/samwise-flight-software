if(BUILD_TESTS)
  # For test builds, create library that links against mocks
  add_library(burn_wire_task burn_wire_task.c)

  # Link against test mocks
  target_link_libraries(burn_wire_task PUBLIC test_mocks)

  # Set include paths: test_mocks first to shadow real hardware headers
  target_include_directories(burn_wire_task PUBLIC
    "${PROJECT_SOURCE_DIR}/src/test_mocks"
    "${PROJECT_SOURCE_DIR}/src/tasks/burn_wire"
  )
else()
  # For embedded builds, use real hardware dependencies
  add_library(burn_wire_task burn_wire_task.c)

  target_link_libraries(burn_wire_task PUBLIC slate common logger burn_wire error utils neopixel)

  # Specify directories with relevant header files
  target_include_directories(burn_wire_task PUBLIC
    "${PROJECT_SOURCE_DIR}/src/tasks/burn_wire"
  )
endif()
