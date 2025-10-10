add_library(scheduler scheduler.c)
target_link_libraries(scheduler PUBLIC
  pico_stdlib
  hardware_spi
  init_state
  running_state
  bringup_state
  burn_wire_state
  burn_wire_reset_state
  slate
  common
  error
)

target_include_directories(scheduler PUBLIC
  "${PROJECT_SOURCE_DIR}/src/scheduler"
)

# Debug message to print the list of include directories for state_machine
get_target_property(INCLUDE_DIRS scheduler INCLUDE_DIRECTORIES)
message("[state_machine] Include directories: ${INCLUDE_DIRS}")

# Add tests subdirectory if in test mode
if(BUILD_TESTS)
  add_subdirectory(test)
endif()
