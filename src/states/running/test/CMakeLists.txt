# Test for actual running_state with real tasks
samwise_add_test(
  NAME running_state_test
  SOURCES test_running_state.c ${PROJECT_SOURCE_DIR}/src/scheduler/scheduler.c
  LIBRARIES test_infrastructure running_state
)
