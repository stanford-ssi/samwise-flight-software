add_compile_definitions(${SAMWISE_DEFINES} COMMIT_HASH="${COMMIT_HASH}")

add_subdirectory(common)
add_subdirectory(slate)
add_subdirectory(error)
add_subdirectory(drivers)
add_subdirectory(scheduler)
add_subdirectory(states)
add_subdirectory(tasks)

# Add the standard include files to the build
# target_include_directories(samwise PRIVATE
#   ${CMAKE_CURRENT_SOURCE_DIR}/src/common # common headers
# )


add_executable(samwise main.c init.c)

pico_set_program_name(samwise samwise)
pico_set_program_version(samwise "0.1")

pico_enable_stdio_uart(samwise 0)
pico_enable_stdio_usb(samwise 1)

# Pico SDK
target_link_libraries(samwise PRIVATE hardware_spi hardware_i2c
  pico_unique_id pico_stdlib)

# Samwise
target_link_libraries(samwise PRIVATE scheduler error rfm9x slate)

# Global include directory common for all targets
target_include_directories(samwise PUBLIC
  "${PROJECT_SOURCE_DIR}/src/common"
  "${PROJECT_SOURCE_DIR}/src/drivers/rfm9x"
  "${PROJECT_SOURCE_DIR}/src/scheduler"
)

pico_add_extra_outputs(samwise)
