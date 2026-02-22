"""Build rule definitions for SAMWISE Flight Software

Provides the samwise_test() macro for automatic dependency remapping from
real hardware drivers to their mock equivalents for unit testing.
"""

# ============================================================================
# Mock Dependency Mappings
# ============================================================================

# Mapping of real driver targets to their mock equivalents
# Format: "real_target" -> "mock_target"
#
# For SAMWISE drivers: Mocks are co-located with the driver in the same directory
# For Pico-SDK hardware: Mocks are centralized in //src/test_mocks/
_MOCK_MAPPINGS = {
    # SAMWISE Drivers (co-located mocks)
    "//src/drivers/logger": "//src/drivers/logger:logger_mock",
    "//src/drivers/logger:logger": "//src/drivers/logger:logger_mock",

    "//src/drivers/flash": "//src/drivers/flash:flash_mock",
    "//src/drivers/flash:flash": "//src/drivers/flash:flash_mock",

    "//src/drivers/rfm9x": "//src/drivers/rfm9x:rfm9x_mock",
    "//src/drivers/rfm9x:rfm9x": "//src/drivers/rfm9x:rfm9x_mock",

    "//src/drivers/neopixel": "//src/drivers/neopixel:neopixel_mock",
    "//src/drivers/neopixel:neopixel": "//src/drivers/neopixel:neopixel_mock",

    "//src/drivers/watchdog": "//src/drivers/watchdog:watchdog_mock",
    "//src/drivers/watchdog:watchdog": "//src/drivers/watchdog:watchdog_mock",

    "//src/drivers/burn_wire": "//src/drivers/burn_wire:burn_wire_mock",
    "//src/drivers/burn_wire:burn_wire": "//src/drivers/burn_wire:burn_wire_mock",

    "//src/drivers/adm1176": "//src/drivers/adm1176:adm1176_mock",
    "//src/drivers/adm1176:adm1176": "//src/drivers/adm1176:adm1176_mock",

    "//src/drivers/mppt": "//src/drivers/mppt:mppt_mock",
    "//src/drivers/mppt:mppt": "//src/drivers/mppt:mppt_mock",

    "//src/drivers/mram": "//src/drivers/mram:mram_mock",
    "//src/drivers/mram:mram": "//src/drivers/mram:mram_mock",

    "//src/drivers/adcs": "//src/drivers/adcs:adcs_mock",
    "//src/drivers/adcs:adcs": "//src/drivers/adcs:adcs_mock",

    "//src/drivers/onboard_led": "//src/drivers/onboard_led:onboard_led_mock",
    "//src/drivers/onboard_led:onboard_led": "//src/drivers/onboard_led:onboard_led_mock",

    "//src/drivers/payload_uart": "//src/drivers/payload_uart:payload_uart_mock",
    "//src/drivers/payload_uart:payload_uart": "//src/drivers/payload_uart:payload_uart_mock",

    "//src/drivers/device_status": "//src/drivers/device_status:device_status_mock",
    "//src/drivers/device_status:device_status": "//src/drivers/device_status:device_status_mock",

    # Core libraries
    "//src/error": "//src/error:error_mock",
    "//src/error:error": "//src/error:error_mock",

    # Pico-SDK Hardware Abstractions (centralized mocks in test_mocks/)
    "@pico-sdk//src/common/pico_stdlib": "//src/test_mocks:pico_stdlib_mock",
    "@pico-sdk//src/common/pico_stdlib:pico_stdlib": "//src/test_mocks:pico_stdlib_mock",

    "@pico-sdk//src/rp2_common/pico_stdlib": "//src/test_mocks:pico_stdlib_mock",
    "@pico-sdk//src/rp2_common/pico_stdlib:pico_stdlib": "//src/test_mocks:pico_stdlib_mock",

    "@pico-sdk//src/common/pico_time": "//src/test_mocks:pico_time_mock",
    "@pico-sdk//src/common/pico_time:pico_time": "//src/test_mocks:pico_time_mock",

    "@pico-sdk//src/rp2_common/hardware_spi": "//src/test_mocks:hardware_spi_mock",
    "@pico-sdk//src/rp2_common/hardware_spi:hardware_spi": "//src/test_mocks:hardware_spi_mock",

    "@pico-sdk//src/rp2_common/hardware_i2c": "//src/test_mocks:hardware_i2c_mock",
    "@pico-sdk//src/rp2_common/hardware_i2c:hardware_i2c": "//src/test_mocks:hardware_i2c_mock",

    "@pico-sdk//src/rp2_common/hardware_uart": "//src/test_mocks:hardware_uart_mock",
    "@pico-sdk//src/rp2_common/hardware_uart:hardware_uart": "//src/test_mocks:hardware_uart_mock",

    "@pico-sdk//src/rp2_common/hardware_pwm": "//src/test_mocks:hardware_pwm_mock",
    "@pico-sdk//src/rp2_common/hardware_pwm:hardware_pwm": "//src/test_mocks:hardware_pwm_mock",

    "@pico-sdk//src/rp2_common/hardware_gpio": "//src/test_mocks:hardware_gpio_mock",
    "@pico-sdk//src/rp2_common/hardware_gpio:hardware_gpio": "//src/test_mocks:hardware_gpio_mock",

    "@pico-sdk//src/rp2_common/hardware_flash": "//src/test_mocks:hardware_flash_mock",
    "@pico-sdk//src/rp2_common/hardware_flash:hardware_flash": "//src/test_mocks:hardware_flash_mock",

    "@pico-sdk//src/rp2_common/hardware_sync": "//src/test_mocks:hardware_sync_mock",
    "@pico-sdk//src/rp2_common/hardware_sync:hardware_sync": "//src/test_mocks:hardware_sync_mock",

    "@pico-sdk//src/rp2_common/hardware_pio": "//src/test_mocks:hardware_pio_mock",
    "@pico-sdk//src/rp2_common/hardware_pio:hardware_pio": "//src/test_mocks:hardware_pio_mock",

    "@pico-sdk//src/rp2_common/hardware_clocks": "//src/test_mocks:hardware_clocks_mock",
    "@pico-sdk//src/rp2_common/hardware_clocks:hardware_clocks": "//src/test_mocks:hardware_clocks_mock",

    "@pico-sdk//src/rp2_common/hardware_resets": "//src/test_mocks:hardware_resets_mock",
    "@pico-sdk//src/rp2_common/hardware_resets:hardware_resets": "//src/test_mocks:hardware_resets_mock",

    "@pico-sdk//src/rp2_common/hardware_irq": "//src/test_mocks:hardware_irq_mock",
    "@pico-sdk//src/rp2_common/hardware_irq:hardware_irq": "//src/test_mocks:hardware_irq_mock",

    "@pico-sdk//src/common/pico_util": "//src/test_mocks:pico_util_mock",
    "@pico-sdk//src/common/pico_util:pico_util": "//src/test_mocks:pico_util_mock",
}

# ============================================================================
# Helper Functions
# ============================================================================

def _remap_deps_to_mocks(deps):
    """Remap driver dependencies to their mock equivalents for testing.

    This function takes a list of dependency labels and replaces any that
    correspond to hardware drivers or pico-sdk hardware abstractions with
    their mock equivalents.

    Args:
        deps: List of dependency labels (strings)

    Returns:
        List of dependencies with drivers/hardware replaced by mocks
    """
    remapped = []
    seen = {}  # Use dict as set to track unique dependencies

    for dep in deps:
        # Check if this dependency has a mock mapping
        mock_dep = _MOCK_MAPPINGS.get(dep, dep)

        # Only add if not already present (avoid duplicates)
        if mock_dep not in seen:
            seen[mock_dep] = True
            remapped.append(mock_dep)

    return remapped

# ============================================================================
# Public Macros
# ============================================================================

def samwise_test(name, srcs, deps = [], copts = [], defines = [], **kwargs):
    """Create a SAMWISE unit test with automatic mock substitution.

    This macro automatically remaps driver dependencies to their mock
    equivalents, so tests can depend on the same targets as production
    code but get mock implementations at link time.

    The macro:
    1. Remaps all driver and pico-sdk hardware dependencies to mocks
    2. Adds the test infrastructure library
    3. Defines TEST=1 for conditional compilation
    4. Sets target_compatible_with to host platform only

    Example usage:
        load("//bzl:defs.bzl", "samwise_test")

        samwise_test(
            name = "radio_test",
            srcs = ["test/radio_test.c"],
            deps = [
                "//src/tasks/radio",        # Depends on real drivers
                "//src/drivers/rfm9x",       # <- Automatically remapped to mock
                "//src/drivers/neopixel",    # <- Automatically remapped to mock
                "@pico-sdk//src/common/pico_stdlib",  # <- Remapped to mock
            ],
        )

    Args:
        name: Name of the test target
        srcs: Test source files (.c files)
        deps: Dependencies (will be automatically remapped to mocks)
        copts: Additional compiler options
        defines: Additional preprocessor defines
        **kwargs: Additional arguments passed to cc_test
    """
    # Remap dependencies to mocks
    test_deps = _remap_deps_to_mocks(deps)

    # Add standard test infrastructure
    if "//src/test_infrastructure" not in test_deps:
        test_deps.append("//src/test_infrastructure")

    # Combine defines with TEST flag
    test_defines = list(defines)
    if "TEST" not in test_defines:
        test_defines.append("TEST")

    # Create the test target
    # Use local_defines (not defines) to avoid propagating TEST to deps,
    # which would create a configuration split with the global --copt=-DTEST=1
    # from --config=tests and cause "conflicting actions" errors.
    native.cc_test(
        name = name,
        srcs = srcs,
        deps = test_deps,
        copts = copts,
        local_defines = test_defines,
        testonly = True,
        **kwargs
    )
