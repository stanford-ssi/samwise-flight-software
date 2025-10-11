if(BUILD_TESTS)
  # For test builds, create library that links against mocks
  add_library(telemetry_task telemetry_task.c)

  # Link against test mocks
  target_link_libraries(telemetry_task PUBLIC test_mocks)

  # Set include paths: test_mocks first to shadow real hardware headers
  target_include_directories(telemetry_task PUBLIC
    "${PROJECT_SOURCE_DIR}/src/test_mocks"
    "${PROJECT_SOURCE_DIR}/src/tasks/telemetry"
  )
else()
  # For embedded builds, use real hardware dependencies
  add_library(telemetry_task telemetry_task.c)

  # Specify targets to link compiled binaries
  target_link_libraries(telemetry_task PUBLIC
    adm1176
    common
    device_status
    error
    hardware_i2c
    logger
    mppt
    neopixel
    pico_stdlib
    rfm9x
    slate
  )

  # Specify directories with relevant header files
  target_include_directories(telemetry_task PUBLIC
    "${PROJECT_SOURCE_DIR}/src/tasks/telemetry"
  )
endif()
