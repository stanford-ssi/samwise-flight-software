enable_testing()

add_executable(beacon_task_test beacon_test.c)
target_link_libraries(beacon_task_test beacon_task)
target_include_directories(beacon_task_test PUBLIC
  "${PROJECT_SOURCE_DIR}/src/tasks/beacon"
)

add_test(NAME BeaconTaskTest COMMAND beacon_task_test)
