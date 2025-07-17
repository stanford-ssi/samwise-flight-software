add_executable(adm1176_test adm1176_test.c)

target_link_libraries(adm1176_test
    adm1176
    hal
)

target_include_directories(adm1176_test PRIVATE
    "${PROJECT_SOURCE_DIR}/src"
    "${PROJECT_SOURCE_DIR}/src/drivers/adm1176"
)