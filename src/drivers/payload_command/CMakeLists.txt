add_library(payload_command payload_command.c)

target_link_libraries(payload_command PRIVATE slate payload_uart)

target_include_directories(payload_command PUBLIC
    "${PROJECT_SOURCE_DIR}/src/drivers/payload_command"
)
