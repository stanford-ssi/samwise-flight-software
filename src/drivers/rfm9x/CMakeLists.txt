add_library(rfm9x rfm9x.c)

target_link_libraries(rfm9x PRIVATE common utils pico_stdlib hardware_spi logger)

target_include_directories(rfm9x PUBLIC 
  "${PROJECT_SOURCE_DIR}/src/drivers/rfm9x"
)
