add_library(bringup_state bringup_state.c)

# Specify targets to link compiled binaries
<<<<<<< HEAD
target_link_libraries(bringup_state PUBLIC common slate diagnostics_task print_task running_state
  init_state)
=======
target_link_libraries(bringup_state PUBLIC
  common
  slate
  diagnostics_task
  watchdog_task
  running_state
  init_state
)
>>>>>>> 915662d (Watchdog (#37))

# Specify directories with relevant header files
target_include_directories(bringup_state PUBLIC
  "${PROJECT_SOURCE_DIR}/src/states/bringup"
)
