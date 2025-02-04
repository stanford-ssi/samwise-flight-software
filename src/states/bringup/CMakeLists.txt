add_library(bringup_state bringup_state.c)

# Specify targets to link compiled binaries
target_link_libraries(bringup_state PUBLIC diagnostics_task running_state
  init_state)

# Specify directories with relevant header files
target_include_directories(bringup_state PUBLIC
  "${PROJECT_SOURCE_DIR}/src/common"
  "${PROJECT_SOURCE_DIR}/src/scheduler"
)
