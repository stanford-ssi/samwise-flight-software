add_library(rfm9x rfm9x.c)

target_link_libraries(rfm9x PRIVATE error pico_stdlib hardware_spi)

target_include_directories(rfm9x PUBLIC 
  "${PROJECT_SOURCE_DIR}/src/common"
  "${PROJECT_SOURCE_DIR}/src/common/error"
)
