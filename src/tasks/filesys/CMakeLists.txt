add_library(filesys_task filesys_task.c)

# Specify targets to link compiled binaries
target_link_libraries(filesys_task PUBLIC slate common logger neopixel mram)

# Specify directories with relevant header files
target_include_directories(filesys_task PUBLIC
  "${PROJECT_SOURCE_DIR}/src/tasks/filesys"
)