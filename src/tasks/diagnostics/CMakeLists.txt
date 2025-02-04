add_library(diagnostics_task diagnostics_task.c)

# Specify targets to link compiled binaries
target_link_libraries(diagnostics_task PUBLIC
  pico_stdlib
  hardware_i2c
  hardware_spi
  pico_unique_id
  error
  rfm9x
)

# Specify directories with relevant header files
target_include_directories(diagnostics_task PUBLIC
  "${PROJECT_SOURCE_DIR}/src/common"
  "${PROJECT_SOURCE_DIR}/src/common/error"
  "${PROJECT_SOURCE_DIR}/src/drivers/rfm9x"

  "${PROJECT_SOURCE_DIR}/src/tasks/diagnostics"
)
