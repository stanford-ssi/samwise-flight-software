add_library(solar fixed_solar.c)

target_link_libraries(solar PRIVATE pico_stdlib)

target_include_directories(fixed_solar PUBLIC 
  "${PROJECT_SOURCE_DIR}/src/drivers/solar"
)