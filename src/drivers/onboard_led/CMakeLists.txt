add_library(onboard_led onboard_led.c)

target_link_libraries(onboard_led PRIVATE pico_stdlib)

target_include_directories(onboard_led PUBLIC 
  "${PROJECT_SOURCE_DIR}/src/drivers/onboard_led"
)
