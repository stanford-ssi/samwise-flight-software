add_library(device_status device_status.c)

target_link_libraries(device_status PRIVATE common pico_stdlib)

target_include_directories(device_status PUBLIC 
  "${PROJECT_SOURCE_DIR}/src/drivers/device_status"
)
