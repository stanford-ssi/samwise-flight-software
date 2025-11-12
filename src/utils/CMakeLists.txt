add_library(utils INTERFACE)

# For test builds, only provide the header (safe_sleep.h uses mocked dependencies)
# For embedded builds, link against real hardware libraries
if(NOT BUILD_TESTS)
  target_link_libraries(utils INTERFACE pico_stdlib slate common watchdog)
endif()

target_include_directories(utils INTERFACE
  "${PROJECT_SOURCE_DIR}/src/utils"
)

if (BUILD_TESTS)
  add_subdirectory(test)
endif()
