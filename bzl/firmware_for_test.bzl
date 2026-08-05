"""Build the picubed-debug firmware .bin from a host-config test.

`firmware_bin_for_testing` wraps a cc_binary (typically `//:samwise`) and
exposes its raw `.bin` image as a regular file usable as `data` on a host
`cc_test`. A Starlark transition switches into the same configuration that
`--config=picubed-debug` sets up in `.bazelrc`, and `pico_bin_aspect` (which
runs under the transitioned config, so it has the arm-none-eabi toolchain)
performs the ELF→bin objcopy.

If you change `build:picubed-debug` in `.bazelrc` (lines 54-65), mirror the
relevant flags here so the transitioned build keeps matching the CLI build.
"""

load("//bzl:bin_aspect.bzl", "pico_bin_aspect")

def _picubed_transition_impl(_settings, _attr):
    # PICO_BOARD/PICO_PLATFORM come from MODULE.bazel (module extension); the
    # build profile is exposed as the //bzl:profile string_flag so we can
    # switch it here (Bazel transitions can't touch --define). Setting it to
    # "picubed-debug" turns off the test-mode select()s that would otherwise
    # pull in the pico-sdk mocks.
    return {
        "//bzl:profile": "picubed-debug",
        "//command_line_option:platforms": "//platforms:arm_cortex_m33_platform",
        "//command_line_option:compilation_mode": "dbg",
        "//command_line_option:copt": ["-DDEBUG=1", "-g", "-Og"],
        "@pico-sdk//bazel/config:PICO_STDIO_USB": True,
    }

_picubed_transition = transition(
    implementation = _picubed_transition_impl,
    inputs = [],
    outputs = [
        "//bzl:profile",
        "//command_line_option:platforms",
        "//command_line_option:compilation_mode",
        "//command_line_option:copt",
        "@pico-sdk//bazel/config:PICO_STDIO_USB",
    ],
)

def _firmware_bin_for_testing_impl(ctx):
    target = ctx.attr.binary[0]
    bin_files = target[OutputGroupInfo].pico_bin_files.to_list()
    if len(bin_files) != 1:
        fail("expected exactly one pico_bin_files output, got: %s" % bin_files)
    bin_file = bin_files[0]
    return [DefaultInfo(
        files = depset([bin_file]),
        runfiles = ctx.runfiles(files = [bin_file]),
    )]

firmware_bin_for_testing = rule(
    implementation = _firmware_bin_for_testing_impl,
    attrs = {
        "binary": attr.label(
            mandatory = True,
            cfg = _picubed_transition,
            aspects = [pico_bin_aspect],
            doc = "cc_binary to build under picubed-debug and convert to .bin.",
        ),
    },
    doc = "Re-exports a firmware .bin built under picubed-debug for use as test data.",
)
