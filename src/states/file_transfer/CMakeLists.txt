add_library(file_transfer_state file_transfer_state.c)

# Specify target libraries to link compiled binaries
target_link_libraries(file_transfer_state PUBLIC common slate watchdog_task radio_task file_transfer_task command_task beacon_task)

# Specify directories with relevant header files
target_include_directories(file_transfer_state PUBLIC "${PROJECT_SOURCE_DIR}/src/states/file_transfer")
