add_library(sd-card_task sd-card_task.c)

# Specify targets to link compiled binaries
target_link_libraries(sd-card_task PUBLIC common slate error sd-card pico_stdlib)

# Specify directories with relevant header files
target_include_directories(sd-card_task PUBLIC
  "${PROJECT_SOURCE_DIR}/src/tasks/sd-card-rw"
)
