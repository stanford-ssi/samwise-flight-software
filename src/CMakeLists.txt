add_subdirectory(common)
add_subdirectory(drivers)
add_subdirectory(scheduler)
add_subdirectory(states)
add_subdirectory(tasks)

add_executable(samwise main.c init.c)
target_link_libraries(samwise PUBLIC scheduler rfm9x)

# Global include directory common for all targets
target_include_directories(samwise PUBLIC
  "${PROJECT_SOURCE_DIR}/src/common"
  "${PROJECT_SOURCE_DIR}/src/drivers/rfm9x"
  "${PROJECT_SOURCE_DIR}/src/scheduler"
)

pico_set_program_name(samwise "samwise")
pico_set_program_version(samwise "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(samwise 0)
pico_enable_stdio_usb(samwise 1)

pico_add_extra_outputs(samwise)
