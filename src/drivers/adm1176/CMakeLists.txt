add_library(adm1176 adm1176.c)

if(PROFILE STREQUAL "TEST")
    target_compile_definitions(adm1176 PUBLIC TEST_MODE)
    target_link_libraries(adm1176 PUBLIC hal error logger)
else()
    target_link_libraries(adm1176 PUBLIC
        pico_stdlib
        hardware_i2c
        hal
        error
    )
endif()

target_include_directories(adm1176 PUBLIC
    "${PROJECT_SOURCE_DIR}/src"
    "${PROJECT_SOURCE_DIR}/src/drivers/adm1176"
)

# Add tests for TEST profile
if(PROFILE STREQUAL "TEST")
    add_subdirectory(test)
endif()
