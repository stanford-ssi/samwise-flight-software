add_library(adcs_task adcs_task.c)

# Specify targets to link compiled binaries
target_link_libraries(adcs_task PUBLIC slate common error adcs_driver)

# Specify directories with relevant header files
target_include_directories(adcs_task PUBLIC
  "${PROJECT_SOURCE_DIR}/src/tasks/adcs"
  "${PROJECT_SOURCE_DIR}/src/drivers/adcs"
)
