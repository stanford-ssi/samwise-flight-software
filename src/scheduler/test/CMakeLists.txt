# Test for core scheduler logic with custom test tasks
samwise_add_test(
  NAME scheduler_test
  SOURCES test_scheduler.c ${PROJECT_SOURCE_DIR}/src/scheduler/scheduler.c
  LIBRARIES test_infrastructure
)
