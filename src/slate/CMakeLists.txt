add_library(slate slate.c)

target_link_libraries(slate PUBLIC pico_stdlib scheduler onboard_led watchdog rfm9x)

target_include_directories(slate PUBLIC
  "${PROJECT_SOURCE_DIR}/src/slate"
  "${PROJECT_SOURCE_DIR}/lib/littlefs-cmake"
)

target_link_libraries(slate PRIVATE littlefs)
