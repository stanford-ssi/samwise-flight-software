add_library(init init.c)

target_link_libraries(init PUBLIC
  burn_wire
  device_status
  hardware_i2c
  neopixel
  onboard_led
  pico_stdlib
  scheduler
  slate
  watchdog
)

target_include_directories(init PUBLIC
  "${PROJECT_SOURCE_DIR}/src/init"
)
