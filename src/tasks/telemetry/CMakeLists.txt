add_library(telemetry_task telemetry_task.c)

# Specify targets to link compiled binaries
target_link_libraries(telemetry_task PUBLIC
  pico_stdlib
  hardware_i2c
  logger
  slate
  common
  error
  rfm9x
  adm1176
  mppt
  neopixel
)

# Specify directories with relevant header files
target_include_directories(telemetry_task PUBLIC
  "${PROJECT_SOURCE_DIR}/src/tasks/telemetry"
)
