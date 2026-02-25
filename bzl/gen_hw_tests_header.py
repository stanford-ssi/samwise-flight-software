#!/usr/bin/env python3
"""Generate hardware_tests.h from a list of integration-test names.

Usage:
    gen_hw_tests_header.py <test_name> [<test_name> ...]

Each <test_name> must match the name used in samwise_integration_test().
The generated header includes hw_test_types.h (for the hw_test_fn typedef and
hw_test_entry_t struct), then declares every <name>_int_main() entry point and
defines HW_TEST_TABLE so hardware_test_task.c needs no manual edits when tests
are added or removed.
"""

import sys


def main():
    test_names = sys.argv[1:]
    if not test_names:
        print("WARNING: no test names supplied; HW_TEST_TABLE will be empty.",
              file=sys.stderr)

    lines = [
        "#pragma once",
        "",
        "#include \"hw_test_types.h\"",
        "",
        "/* ── Entry-point declarations (auto-generated) ────────────────────── */",
        "",
    ]

    for name in test_names:
        lines.append(f"void {name}_int_main(void);")

    lines += [
        "",
        "/* ── Test table (auto-generated) ──────────────────────────────────── */",
        "",
        "#define HW_TEST_TABLE  \\",
        "    {                  \\",
    ]

    for name in test_names:
        lines.append(f'        {{"{name}", {name}_int_main}}, \\')

    lines.append("    }")

    print("\n".join(lines))


if __name__ == "__main__":
    main()
