add_library(watchdog watchdog.c)

target_link_libraries(watchdog PRIVATE common error pico_stdlib)

target_include_directories(watchdog PUBLIC 
  "${PROJECT_SOURCE_DIR}/src/drivers/watchdog"
)
