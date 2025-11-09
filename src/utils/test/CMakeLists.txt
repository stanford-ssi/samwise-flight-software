# Test for str utils logic with custom test tasks
samwise_add_test(
  NAME str_utils_test
  SOURCES test_str_utils.c ${PROJECT_SOURCE_DIR}/src/utils/str_utils.h
  LIBRARIES utils
)
