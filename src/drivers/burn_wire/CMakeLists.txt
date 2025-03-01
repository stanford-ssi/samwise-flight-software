add_library(burn_wire burn_wire.c)

target_link_libraries(burn_wire PUBLIC pico_stdlib hardware_pwm)


target_include_directories(burn_wire PUBLIC
  "${PROJECT_SOURCE_DIR}/src/drivers/burn_wire"
)
