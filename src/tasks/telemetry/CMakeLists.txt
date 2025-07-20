add_library(telemetry_task telemetry_task.c)

# Specify targets to link compiled binaries
target_link_libraries(telemetry_task PUBLIC
  adm1176
  common
  device_status
  error
  hardware_i2c
  logger
  mppt
  pico_stdlib
  rfm9x
  slate
)

# Specify directories with relevant header files
target_include_directories(telemetry_task PUBLIC
  "${PROJECT_SOURCE_DIR}/src/tasks/telemetry"
)
