"""Build rule definitions for SAMWISE Flight Software

Provides:
  samwise_test()                    — host unit test with automatic mock substitution.
  samwise_integration_test()        — hardware integration test; creates both a
                                      firmware-linkable hw_lib and a host unit test.
  hardware_integration_test_suite() — collects integration tests and auto-generates
                                      hardware_tests.h (replaces the hand-maintained
                                      file of the same name).
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
    if "//src/test_infrastructure:test_infrastructure" not in test_deps:
        test_deps.append("//src/test_infrastructure:test_infrastructure")

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

def samwise_integration_test(name, int_src, srcs = [], deps = [], copts = [], defines = [], **kwargs):
    """Register a SAMWISE hardware integration test.

    Creates ONE target:

    ``<name>_hw_lib`` — a cc_library that can be linked into the firmware
    BRINGUP binary by ``hardware_integration_test_suite()``.

    The ``int_src`` file contains the integration entry point as a plain
    ``main()`` function.  The macro compiles it with ``-Dmain=<name>_int_main``
    so that multiple tests can coexist in the same binary.

    Any additional ``srcs`` (helper / shared test files that may contain their
    own standalone ``main()``) are compiled with ``-Dmain=_unused_main_`` so
    their ``main()`` is harmlessly renamed — **no ``#ifdef`` guards needed**.

    Example:

        load("//bzl:defs.bzl", "samwise_integration_test", "samwise_test")

        samwise_integration_test(
            name  = "mram_test",
            int_src = "mram_integration_test.c",
            srcs  = ["mram_test.c"],
            hdrs  = ["mram_test.h"],
            deps  = [...],
        )

        samwise_test(
            name = "mram_test",
            srcs = ["mram_test.c"],
            deps = [...],
        )

    To enrol this test in the hardware runner, add its name and target to the
    ``tests`` dict of the ``hardware_integration_test_suite()`` call in
    ``src/tasks/hardware_test/BUILD.bazel``:

        hardware_integration_test_suite(
            name = "hardware_test_lib",
            tests = {
                "mram_test": "//src/filesys/test:mram_test_hw_lib",
            },
        )

    Args:
        name:    Logical test name; also used as the ``<name>_int_main`` symbol.
        int_src: The integration entry-point source file.  Its ``main()`` is
                 renamed to ``<name>_int_main`` via the preprocessor.
        srcs:    Helper / shared source files.  Any ``main()`` they contain is
                 harmlessly neutralised (renamed to ``_unused_main_``).
        deps:    Dependencies forwarded to the hw_lib cc_library.
        copts:   Additional compiler options.
        defines: Additional preprocessor defines.
        **kwargs: Extra arguments forwarded to cc_library.
    """

    # cc_test does not support `hdrs`; pop it from kwargs before forwarding.
    hdrs = kwargs.pop("hdrs", [])
    
    deps = list(deps)  # Make a mutable copy of deps
    # Add standard test infrastructure
    if "//src/test_infrastructure:hardware_test_infrastructure" not in deps:
        deps.append("//src/test_infrastructure:hardware_test_infrastructure")

    # ── 1. Helper sources ─────────────────────────────────────────────────
    # Compile helper files (which may contain a standalone main()) with main
    # renamed to a throwaway symbol so it never conflicts.
    helpers_lib = None
    if srcs:
        helpers_lib = name + "_hw_helpers"
        native.cc_library(
            name = helpers_lib,
            srcs = srcs,
            hdrs = hdrs,
            deps = deps,
            copts = copts,
            defines = defines,
            local_defines = ["main=_unused_" + name + "_main_"],
            **kwargs
        )

    # ── 2. Integration entry point ────────────────────────────────────────
    # Compile the entry-point source with main() renamed to <name>_int_main()
    # so that multiple integration tests can be linked into the same binary.
    hw_deps = list(deps)
    if helpers_lib:
        hw_deps.append(":" + helpers_lib)

    native.cc_library(
        name = name + "_hw_lib",
        srcs = [int_src],
        hdrs = hdrs,
        deps = hw_deps,
        copts = copts,
        defines = defines,
        local_defines = ["main=" + name + "_int_main"],
        **kwargs
    )

def hardware_integration_test_suite(name, tests):
    """Collect integration tests and emit a self-contained hardware_tests.h.

    This macro replaces the hand-maintained ``hardware_tests.h`` file.  It:

    1. Runs a genrule that calls ``//bzl:gen_hw_tests_header`` with the
       registered test names to produce ``hardware_tests.h``.
    2. Bundles the generated header together with all ``_hw_lib`` targets into
       a single cc_library whose name is *name*.  The ``hardware_test_task``
       target should depend on *name* exactly as it previously depended on the
       static ``hardware_test_lib``.

    ``hardware_test_task.c`` requires no changes — it continues to include
    ``"test/hardware_tests.h"`` and expand ``HW_TEST_TABLE``.

    Example usage (in ``src/tasks/hardware_test/test/BUILD.bazel``):

        load("//bzl:defs.bzl", "hardware_integration_test_suite")

        hardware_integration_test_suite(
            name = "hardware_test_lib",
            tests = {
                "mram":   "//src/filesys:mram_hw_lib",
                "filesys": "//src/filesys:filesys_hw_lib",
            },
        )

    Args:
        name:       Name of the resulting cc_library (consumed by the task).
        tests:      Dict mapping logical test name → Bazel target of the
                    corresponding ``<name>_hw_lib`` cc_library produced by
                    ``samwise_integration_test()``.
                    e.g. ``{"mram": "//src/filesys:mram_hw_lib"}``
    """

    test_names = tests.keys()
    test_targets = tests.values()

    # ── 1. Generate hardware_tests.h ─────────────────────────────────────
    # Pass all test names as positional arguments to the generator script.
    # The script writes the header to stdout which the genrule redirects to $@.
    native.genrule(
        name = name + "_gen_header",
        srcs = [],
        outs = ["hardware_tests.h"],
        cmd = "$(location //bzl:gen_hw_tests_header) " +
              " ".join(test_names) + " > $@",
        tools = ["//bzl:gen_hw_tests_header"],
    )

    # ── 2. Bundle header + test libraries ────────────────────────────────
    # includes = ["."] ensures consumers can reach hardware_tests.h via the
    # same relative path ("test/hardware_tests.h") that hardware_test_task.c
    # already uses.
    # //bzl:hw_test_types provides the static hw_test_fn/hw_test_entry_t
    # definitions that the generated header includes.
    native.cc_library(
        name = name,
        hdrs = [":" + name + "_gen_header"],
        includes = ["."],
        deps = list(test_targets) + ["//bzl:hw_test_types"],
    )
