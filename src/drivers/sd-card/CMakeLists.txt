add_library(sd-card sd-card.c)
add_subdirectory(no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/src)

target_link_libraries(sd-card PRIVATE no-OS-FatFS-SD-SDIO-SPI-RPi-Pico)

target_include_directories(sd-card PUBLIC 
  "${PROJECT_SOURCE_DIR}/src/drivers/sd-card"
  "$<TARGET_PROPERTY:no-OS-FatFS-SD-SDIO-SPI-RPi-Pico,INTERFACE_INCLUDE_DIRECTORIES>"
)
