add_library(flash flash.c)

target_link_libraries(flash PRIVATE common pico_stdlib hardware_flash
  hardware_sync logger)

target_include_directories(flash PUBLIC 
  "${PROJECT_SOURCE_DIR}/src/drivers/flash"
)
