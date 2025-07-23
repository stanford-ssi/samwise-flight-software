add_library(file_transfer_task file_transfer_task.c)

target_link_libraries(file_transfer_task PUBLIC slate common logger utils protocols)

target_include_directories(file_transfer_task PUBLIC "${PROJECT_SOURCE_DIR}/src/tasks/file_transfer")
