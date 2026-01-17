if(BUILD_TESTS)
  add_library(mram mram.c)
  target_link_libraries(mram PUBLIC test_mocks)
  target_include_directories(mram PRIVATE
    "${PROJECT_SOURCE_DIR}/src/test_mocks"
    "${PROJECT_SOURCE_DIR}/src/drivers/mram"
  )
  add_subdirectory(test)
else()
  add_library(mram mram.c)
  target_link_libraries(mram PUBLIC slate common error pico_stdlib)
  target_include_directories(mram PRIVATE
    "${PROJECT_SOURCE_DIR}/src/drivers/mram"
  )
endif()

