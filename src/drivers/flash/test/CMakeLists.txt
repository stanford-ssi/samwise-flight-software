add_executable(flash_test flash_test.c)

# Link only to hal directly to avoid transitive dependencies
target_link_libraries(flash_test PRIVATE hal)
target_compile_definitions(flash_test PRIVATE TEST_MODE)

# Include flash driver headers
target_include_directories(flash_test PRIVATE 
    "${PROJECT_SOURCE_DIR}/src/drivers/flash"
)