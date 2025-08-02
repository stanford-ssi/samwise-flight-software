add_library(adcs_driver adcs_driver.c)

target_link_libraries(adcs_driver
    pico_stdlib
    hardware_uart
    common
)

target_include_directories(adcs_driver PUBLIC ${CMAKE_CURRENT_LIST_DIR})