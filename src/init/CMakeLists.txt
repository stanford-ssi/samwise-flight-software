add_library(init init.c)

target_link_libraries(init PUBLIC burn_wire pico_stdlib scheduler onboard_led watchdog slate)

target_include_directories(init PUBLIC
  "${PROJECT_SOURCE_DIR}/src/init"
)
