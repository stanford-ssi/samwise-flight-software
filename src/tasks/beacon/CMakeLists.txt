add_library(beacon_task beacon_task.c)

# Specify targets to link compiled binaries
target_link_libraries(beacon_task PUBLIC common slate error pico_stdlib hardware_spi rfm9x
  command_task)

# Specify directories with relevant header files
target_include_directories(beacon_task PUBLIC
  "${PROJECT_SOURCE_DIR}/src/tasks/beacon"
)
