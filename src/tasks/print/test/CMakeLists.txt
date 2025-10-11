# Example test using the samwise_add_test helper
# Developers only need to specify the test name, source files, and libraries to test
samwise_add_test(
  NAME print_task_test
  SOURCES print_test.c
  LIBRARIES print_task
)
