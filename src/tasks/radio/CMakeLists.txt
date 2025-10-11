if(BUILD_TESTS)
  # For test builds, create library that links against mocks
  add_library(radio_task radio_task.c)

  # Link against test mocks
  target_link_libraries(radio_task PUBLIC test_mocks)

  # Set include paths: test_mocks first to shadow real hardware headers
  target_include_directories(radio_task PUBLIC
    "${PROJECT_SOURCE_DIR}/src/test_mocks"
    "${PROJECT_SOURCE_DIR}/src/tasks/radio"
  )
else()
  # For embedded builds, use real hardware dependencies
  add_library(radio_task radio_task.c)

  # Specify targets to link compiled binaries
  target_link_libraries(radio_task PUBLIC common slate packet error pico_stdlib hardware_spi rfm9x neopixel)

  # Specify directories with relevant header files
  target_include_directories(radio_task PUBLIC
    "${PROJECT_SOURCE_DIR}/src/tasks/radio"
  )
endif()
