# Tasks that have test support or are hardware-independent
# (Currently none - most tasks require hardware dependencies)

# Only build hardware-dependent tasks for embedded targets
if(NOT PROFILE STREQUAL "TEST")
    add_subdirectory(adcs)
    add_subdirectory(beacon)
    add_subdirectory(blink)
    add_subdirectory(burn_wire)
    add_subdirectory(command)
    add_subdirectory(diagnostics)
    add_subdirectory(payload)
    add_subdirectory(print)
    add_subdirectory(radio)
    add_subdirectory(telemetry)
    add_subdirectory(watchdog)
endif()
