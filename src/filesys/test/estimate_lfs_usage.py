#!/usr/bin/env python3
# lfs_estimator.py

"""
A command-line tool to estimate the flash storage used by a single file in LittleFS.

This script calculates the total overhead based on the filesystem configuration
and the size of the file's data and custom attributes.

How to Use:
------------
The script requires 3 positional arguments: block_size, prog_size, and file_data_size.
All values should be in bytes.

1. Basic Usage (a 100-byte file):
   python lfs_estimator.py 4096 256 100

2. With Custom Attributes (an empty file with two attributes of 16 and 32 bytes):
   python lfs_estimator.py 4096 256 0 --attributes 16 32

3. For a different flash configuration (e.g., smaller blocks):
   python lfs_estimator.py 512 256 50 --attributes 8
"""

import math
import argparse
import sys

# --- Configuration Constants for LittleFS ---
CTZ_POINTER_SIZE = 4
ATTRIBUTE_TAG_OVERHEAD = 8
INTRINSIC_METADATA_OVERHEAD = 64

def estimate_lfs_file_size(
    file_data_size: int,
    block_size: int,
    prog_size: int,
    attributes: list[int] = None
) -> dict:
    """
    Estimates the total flash space consumed by a single file in LittleFS.
    (Core calculation logic remains the same)
    """
    if block_size <= 0 or prog_size <= 0:
        raise ValueError("Block size and program size must be positive integers.")

    metadata_usage = 2 * block_size

    total_attribute_storage_size = 0
    if attributes:
        unaligned_attr_size = sum(ATTRIBUTE_TAG_OVERHEAD + size for size in attributes)
        total_attribute_storage_size = math.ceil(unaligned_attr_size / prog_size) * prog_size

    required_metadata_space = total_attribute_storage_size + INTRINSIC_METADATA_OVERHEAD
    if required_metadata_space > block_size:
        raise ValueError(
            f"Attributes are too large to fit in a metadata block. "
            f"Required: ~{required_metadata_space} bytes, Available: {block_size} bytes."
        )

    data_usage = 0
    if file_data_size > 0:
        usable_space_per_block = block_size - CTZ_POINTER_SIZE
        if usable_space_per_block <= 0:
            raise ValueError(f"Block size ({block_size}) is too small to hold data and a pointer.")
        num_data_blocks = math.ceil(file_data_size / usable_space_per_block)
        data_usage = num_data_blocks * block_size

    total_usage = metadata_usage + data_usage

    return {
        "metadata_usage": metadata_usage,
        "attribute_usage": total_attribute_storage_size,
        "data_usage": data_usage,
        "total_usage": total_usage,
    }

def main():
    """Parses command-line arguments and prints the storage estimation."""
    parser = argparse.ArgumentParser(
        description="Estimate the flash storage used by a single file in LittleFS.",
        formatter_class=argparse.RawTextHelpFormatter, # To preserve help text formatting
        epilog=__doc__ # Use the script's docstring for detailed help
    )

    # --- Define Command-Line Arguments ---
    parser.add_argument(
        "block_size",
        type=int,
        help="The configured block_size of the filesystem in bytes (e.g., 4096)."
    )
    parser.add_argument(
        "prog_size",
        type=int,
        help="The minimum programmable size of the flash in bytes (e.g., 256)."
    )
    parser.add_argument(
        "file_data_size",
        type=int,
        help="The size of the file's data content in bytes."
    )
    parser.add_argument(
        "-a", "--attributes",
        type=int,
        nargs='*',  # 0 or more arguments
        default=[],
        help="A list of custom attribute sizes in bytes (e.g., --attributes 16 32)."
    )

    args = parser.parse_args()

    # --- Run Estimation and Print Results ---
    try:
        usage = estimate_lfs_file_size(
            file_data_size=args.file_data_size,
            block_size=args.block_size,
            prog_size=args.prog_size,
            attributes=args.attributes
        )

        print("\n--- LittleFS Storage Estimation ---")
        print(f"Configuration:")
        print(f"  Block Size:   {args.block_size} bytes")
        print(f"  Program Size: {args.prog_size} bytes")
        print(f"\nFile Definition:")
        print(f"  Data Size:    {args.file_data_size} bytes")
        print(f"  Attributes:   {len(args.attributes)} ({args.attributes} bytes)")

        print("\n--- Usage Breakdown ---")
        print(f"  Metadata Allocation: {usage['metadata_usage']:>7} bytes (The fixed 2*block_size cost)")
        if usage['attribute_usage'] > 0:
            print(f"  └─ Attribute Use:     {usage['attribute_usage']:>7} bytes (Space used *inside* metadata blocks)")
        print(f"  Data Allocation:     {usage['data_usage']:>7} bytes")
        print(f"  ---------------------------------")
        print(f"  Total Flash Usage:   {usage['total_usage']:>7} bytes ({usage['total_usage']/1024:.2f} KiB)")
        print("-" * 35)

    except ValueError as e:
        print(f"\nError: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
