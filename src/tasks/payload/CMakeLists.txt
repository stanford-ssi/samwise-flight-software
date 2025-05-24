add_library(payload_task payload_task.c)

target_link_libraries(payload_task PUBLIC payload_uart slate common logger payload_command)

target_include_directories(payload_task PUBLIC
    "${PROJECT_SOURCE_DIR}/src/tasks/payload"
)
