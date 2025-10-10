# Test for scheduler with custom states defined in test
samwise_add_test(
  NAME running_state_test
  SOURCES test_running_state.c ${PROJECT_SOURCE_DIR}/src/scheduler/scheduler.c
  LIBRARIES 
)
