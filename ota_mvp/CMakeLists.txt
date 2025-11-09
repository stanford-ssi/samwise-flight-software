if(NOT BUILD_TESTS)
  add_executable(ota main.c)

  if(VERSION STREQUAL "BLINK")
    pico_set_program_name(ota ota_blink)
    pico_set_program_version(ota "0.1")
    target_compile_definitions(ota PRIVATE BUILD_BLINK)
  else()
    pico_set_program_name(ota ota_demo)
    pico_set_program_version(ota "0.0")
  endif()

  pico_enable_stdio_usb(ota 1)

  # Pico SDK
  target_link_libraries(ota PRIVATE hardware_spi hardware_i2c pico_stdlib)

  pico_add_extra_outputs(ota)
endif()

