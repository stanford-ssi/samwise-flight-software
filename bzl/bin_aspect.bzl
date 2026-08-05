"""Aspect for generating raw .bin images from ELF binaries.

Mirrors the pico-sdk UF2 aspect but produces raw binary files using
arm-none-eabi-objcopy from the CC toolchain.  Wire it up in .bazelrc:

    build:picubed-debug --aspects=//bzl:bin_aspect.bzl%pico_bin_aspect
    build:picubed-debug --output_groups=+pico_bin_files
"""

_SUPPORTED_BINARY_TYPES = ",".join([
    "cc_binary",
    "cc_test",
])

def _pico_bin_aspect_impl(target, ctx):
    allowed_types = ctx.attr.from_rules.split(",")
    if ctx.rule.kind not in allowed_types and "*" not in allowed_types:
        return []

    cc_toolchain = ctx.attr._cc_toolchain[cc_common.CcToolchainInfo]

    binary_to_convert = target[DefaultInfo].files_to_run.executable
    bin_output = ctx.actions.declare_file(binary_to_convert.basename + ".bin")

    ctx.actions.run_shell(
        outputs = [bin_output],
        inputs = depset([binary_to_convert], transitive = [cc_toolchain.all_files]),
        command = """
            OBJCOPY=$(find . -name 'arm-none-eabi-objcopy' 2>/dev/null | head -1)
            if [ -z "$OBJCOPY" ]; then
                echo "ERROR: Could not find arm-none-eabi-objcopy" >&2
                exit 1
            fi
            "$OBJCOPY" -O binary "$1" "$2"
        """,
        arguments = [binary_to_convert.path, bin_output.path],
        mnemonic = "ElfToBin",
        progress_message = "Converting %s to raw binary" % binary_to_convert.basename,
    )
    return [
        OutputGroupInfo(
            pico_bin_files = depset([bin_output]),
        ),
    ]

pico_bin_aspect = aspect(
    implementation = _pico_bin_aspect_impl,
    doc = """An aspect for generating raw .bin images from ELF binaries.

Produces raw binary files suitable for flashing with picotool or SWD.
Use alongside the UF2 aspect:

    bazel build --config=picubed-debug \\
        --aspects=//bzl:bin_aspect.bzl%pico_bin_aspect \\
        --output_groups=+pico_bin_files \\
        //:samwise
""",
    attrs = {
        "from_rules": attr.string(
            default = _SUPPORTED_BINARY_TYPES,
            values = [_SUPPORTED_BINARY_TYPES, "*"],
            doc = "Comma-separated list of rule kinds to apply the bin aspect to",
        ),
        "_cc_toolchain": attr.label(
            default = "@bazel_tools//tools/cpp:current_cc_toolchain",
        ),
    },
    fragments = ["cpp"],
    toolchains = ["@bazel_tools//tools/cpp:toolchain_type"],
)
