add_executable(print_task_test print_test.c)

target_link_libraries(print_task_test PRIVATE
  print_task
  test_mocks
)

target_include_directories(print_task_test PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${PROJECT_SOURCE_DIR}/src/test_mocks"
  "${PROJECT_SOURCE_DIR}/src/tasks/print"
)

add_test(NAME PrintTaskTest COMMAND print_task_test)
