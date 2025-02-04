enable_testing()

add_executable(print_task_test print_test.c)
target_link_libraries(print_task_test print_task)
target_include_directories(print_task_test PUBLIC
  "${PROJECT_SOURCE_DIR}/src/tasks/print"
)

add_test(NAME PrintTaskTest COMMAND print_task_test)
