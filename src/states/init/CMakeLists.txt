add_library(init_state init_state.c)

# Specify targets to link compiled binaries
target_link_libraries(init_state PUBLIC error hardware_spi running_state)

# Specify directories with relevant header files
target_include_directories(init_state PUBLIC
  "${PROJECT_SOURCE_DIR}/src/common"
  "${PROJECT_SOURCE_DIR}/src/common/error"
  "${PROJECT_SOURCE_DIR}/src/drivers/rfm9x"
  "${PROJECT_SOURCE_DIR}/src/states/running"
  "${PROJECT_SOURCE_DIR}/src/tasks/print"
)
