add_library(fixed_solar fixed_solar.c)

target_link_libraries(fixed_solar PRIVATE pico_stdlib)

target_include_directories(fixed_solar PUBLIC 
  "${PROJECT_SOURCE_DIR}/src/drivers/fixed_solar"
)
