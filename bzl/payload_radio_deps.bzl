"""Module extension for payload/radio (LoRa Linux binaries) external dependencies."""

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def _payload_radio_deps_impl(_ctx):
    # sx128x_driver - Semtech SX128x LoRa driver
    http_archive(
        name = "sx128x_driver",
        url = "https://github.com/YukiWorkshop/sx128x_driver/archive/refs/tags/v0.0.4.tar.gz",
        strip_prefix = "sx128x_driver-0.0.4",
        integrity = "sha256-LfaLjvXIkVc9JhdbVVcAWY+feh7pwgRx7hZXOZox1Lc=",
        build_file_content = """
package(default_visibility = ["//visibility:public"])
cc_library(
    name = "sx128x_driver",
    srcs = ["SX128x.cpp"],
    hdrs = ["SX128x.hpp"],
    includes = ["."],
    strip_include_prefix = ".",
)
""",
    )

    # GPIOPlusPlus - Linux GPIO library
    http_archive(
        name = "gpio_plus_plus",
        url = "https://github.com/YukiWorkshop/GPIOPlusPlus/archive/refs/tags/v0.0.2.tar.gz",
        strip_prefix = "GPIOPlusPlus-0.0.2",
        integrity = "sha256-dqAz42ooga3+Llxkp2yAceZIKsf9XEdYSZ8dFH1VwzM=",
        build_file_content = """
package(default_visibility = ["//visibility:public"])
cc_library(
    name = "gpio_plus_plus",
    srcs = ["GPIO++.cpp", "Utils.cpp"],
    hdrs = ["GPIO++.hpp", "Utils.hpp"],
    includes = ["."],
    strip_include_prefix = ".",
    deps = [],
)
""",
    )

    # SPPI - Linux SPI library
    http_archive(
        name = "sppi",
        url = "https://github.com/YukiWorkshop/SPPI/archive/refs/tags/v0.0.5.tar.gz",
        strip_prefix = "SPPI-0.0.5",
        integrity = "sha256-//DsF/+iGxbvA4cTtYpTYlRzMsQR5nf2fXkYIe0rBpA=",
        build_file_content = """
package(default_visibility = ["//visibility:public"])
cc_library(
    name = "sppi",
    srcs = ["SPPI.cpp"],
    hdrs = ["SPPI.hpp"],
    includes = ["."],
    strip_include_prefix = ".",
)
""",
    )

payload_radio_deps = module_extension(
    implementation = _payload_radio_deps_impl,
)
