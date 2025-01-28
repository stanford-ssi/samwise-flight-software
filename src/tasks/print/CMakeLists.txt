add_library(print_task print_task.c)

# Specify targets to link compiled binaries
target_link_libraries(print_task PUBLIC error rfm9x hardware_spi)

# Specify directories with relevant header files
target_include_directories(print_task PUBLIC
  "${PROJECT_SOURCE_DIR}/src/common"
  "${PROJECT_SOURCE_DIR}/src/common/error"
  "${PROJECT_SOURCE_DIR}/src/drivers/rfm9x"
)

add_subdirectory(test)
