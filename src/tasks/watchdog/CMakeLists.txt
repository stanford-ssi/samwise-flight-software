add_library(watchdog_task watchdog_task.c)

# Specify targets to link compiled binaries
target_link_libraries(watchdog_task PUBLIC common slate error pico_stdlib hardware_spi rfm9x
  command_task)

# Specify directories with relevant header files
target_include_directories(watchdog_task PUBLIC
  "${PROJECT_SOURCE_DIR}/src/tasks/watchdog"
)
