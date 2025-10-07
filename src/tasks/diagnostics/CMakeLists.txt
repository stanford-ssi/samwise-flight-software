if(BUILD_TESTS)
  # For test builds, create library that links against mocks
  add_library(diagnostics_task diagnostics_task.c)

  # Link against test mocks
  target_link_libraries(diagnostics_task PUBLIC test_mocks)

  # Set include paths: test_mocks first to shadow real hardware headers
  target_include_directories(diagnostics_task PUBLIC
    "${PROJECT_SOURCE_DIR}/src/test_mocks"
    "${PROJECT_SOURCE_DIR}/src/tasks/diagnostics"
  )
else()
  # For embedded builds, use real hardware dependencies
  add_library(diagnostics_task diagnostics_task.c)

  # Specify targets to link compiled binaries
  target_link_libraries(diagnostics_task PUBLIC
    pico_stdlib
    hardware_i2c
    hardware_spi
    pico_unique_id
    logger
    slate
    common
    error
    rfm9x
    adm1176
  )

  # Specify directories with relevant header files
  target_include_directories(diagnostics_task PUBLIC
    "${PROJECT_SOURCE_DIR}/src/tasks/diagnostics"
  )
endif()
