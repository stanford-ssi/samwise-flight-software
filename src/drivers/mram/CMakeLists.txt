add_library(mram mram.c)

target_link_libraries(mram PUBLIC slate common error pico_stdlib)

target_include_directories(mram PUBLIC
  "${PROJECT_SOURCE_DIR}/src/drivers/mram"
)

