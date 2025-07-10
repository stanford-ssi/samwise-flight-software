add_library(utils INTERFACE)

target_link_libraries(utils INTERFACE pico_stdlib slate common watchdog hal)

target_include_directories(utils INTERFACE 
  "${PROJECT_SOURCE_DIR}/src/utils"
)
