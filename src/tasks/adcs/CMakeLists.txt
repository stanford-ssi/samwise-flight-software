if(BUILD_TESTS)
  # For test builds, create library that links against mocks
  add_library(adcs_task adcs_task.c)

  # Link against test mocks
  target_link_libraries(adcs_task PUBLIC test_mocks)

  # Set include paths: test_mocks first to shadow real hardware headers
  target_include_directories(adcs_task PUBLIC
    "${PROJECT_SOURCE_DIR}/src/test_mocks"
    "${PROJECT_SOURCE_DIR}/src/tasks/adcs"
    "${PROJECT_SOURCE_DIR}/src/drivers/adcs"
    "${PROJECT_SOURCE_DIR}/src/common"
  )
else()
  # For embedded builds, use real hardware dependencies
  add_library(adcs_task adcs_task.c)

  # Specify targets to link compiled binaries
  target_link_libraries(adcs_task PUBLIC slate common error adcs_driver)

  # Specify directories with relevant header files
  target_include_directories(adcs_task PUBLIC
    "${PROJECT_SOURCE_DIR}/src/tasks/adcs"
    "${PROJECT_SOURCE_DIR}/src/drivers/adcs"
  )
endif()
