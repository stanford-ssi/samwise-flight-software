#!/usr/bin/env python3
"""
Simple CLI to take a photo with optional custom parameters.
Run from the payload directory: python take_photo_cli.py

Interactive mode (no args):
    python take_photo_cli.py

CLI args mode:
    python take_photo_cli.py IMAGE_ID [--cam-num 0] [--ssdv] [options...]
"""

import argparse

import commands
import helpers.ssdv_utils as ssdv_utils

DEFAULTS = {
    "cam_num": 0,
    "camera_name": "A",
    "config": "default",
    "w": 800,
    "h": 600,
    "quality": 100,
    "cells_x": 1,
    "cells_y": 1,
}


def prompt_int(prompt: str, default: int) -> int:
    raw = input(f"{prompt} [{default}]: ").strip()
    return int(raw) if raw else default


def prompt_str(prompt: str, default: str) -> str:
    raw = input(f"{prompt} [{default}]: ").strip()
    return raw if raw else default


def run_photo(image_id: str, cam_num: int, camera_name: str, config: str,
              w: int, h: int, quality: int, cells_x: int, cells_y: int):
    """Take photo and return result tuple."""
    result = commands.take_photo(
        image_id,
        cam_num=cam_num,
        camera_name=camera_name,
        config=config,
        w=w,
        h=h,
        quality=quality,
        cells_x=cells_x,
        cells_y=cells_y,
    )
    raw_size, compressed_size, avg_cell_size, max_cell_size = result
    print(f"\nRaw size: {raw_size} bytes")
    print(f"Compressed size: {compressed_size} bytes")
    print(f"Avg cell size: {avg_cell_size} bytes")
    print(f"Max cell size: {max_cell_size} bytes")
    return raw_size, compressed_size, avg_cell_size, max_cell_size


def main_interactive():
    """Interactive mode: prompt for all inputs."""
    image_id = input("Image ID: ").strip()
    if not image_id:
        print("Image ID is required.")
        return

    use_custom = input("Use custom parameters? (y/N): ").strip().lower() == "y"
    if use_custom:
        cam_num = prompt_int("Camera number", DEFAULTS["cam_num"])
        camera_name = prompt_str("Camera name", DEFAULTS["camera_name"])
        config = prompt_str("Config profile", DEFAULTS["config"])
        w = prompt_int("Width", DEFAULTS["w"])
        h = prompt_int("Height", DEFAULTS["h"])
        quality = prompt_int("Quality (1-100)", DEFAULTS["quality"])
        cells_x = prompt_int("Cells X", DEFAULTS["cells_x"])
        cells_y = prompt_int("Cells Y", DEFAULTS["cells_y"])
    else:
        cam_num, camera_name, config = DEFAULTS["cam_num"], DEFAULTS["camera_name"], DEFAULTS["config"]
        w, h, quality = DEFAULTS["w"], DEFAULTS["h"], DEFAULTS["quality"]
        cells_x, cells_y = DEFAULTS["cells_x"], DEFAULTS["cells_y"]

    run_photo(image_id, cam_num, camera_name, config, w, h, quality, cells_x, cells_y)

    use_ssdv = input("Encode with SSDV? (y/N): ").strip().lower() == "y"
    if use_ssdv:
        image_path = f"{commands.IMAGES_DIR}/{image_id}.jpg"
        ssdv_path = ssdv_utils.encode_file(image_path)
        print(f"SSDV encoded to: {ssdv_path}")

def main_cli(args: argparse.Namespace):
    """CLI args mode: use parsed arguments."""
    run_photo(
        args.image_id,
        args.cam_num,
        args.camera_name,
        args.config,
        args.w,
        args.h,
        args.quality,
        args.cells_x,
        args.cells_y,
    )
    if args.ssdv:
        image_path = f"{commands.IMAGES_DIR}/{args.image_id}.jpg"
        ssdv_path = ssdv_utils.encode_file(image_path)
        print(f"SSDV encoded to: {ssdv_path}")


def main():
    parser = argparse.ArgumentParser(
        description="Take a photo with optional custom parameters.",
        epilog="Run with no arguments for interactive mode.",
    )
    parser.add_argument(
        "image_id",
        nargs="?",
        help="Image ID (omit for interactive mode)",
    )
    parser.add_argument("--cam-num", type=int, default=DEFAULTS["cam_num"], help="Camera number")
    parser.add_argument("--camera-name", default=DEFAULTS["camera_name"], help="Camera name")
    parser.add_argument("--config", default=DEFAULTS["config"], help="Config profile")
    parser.add_argument("-w", "--width", type=int, default=DEFAULTS["w"], dest="w", help="Width")
    parser.add_argument("-H", "--height", type=int, default=DEFAULTS["h"], dest="h", help="Height")
    parser.add_argument("-q", "--quality", type=int, default=DEFAULTS["quality"], help="Quality 1-100")
    parser.add_argument("--cells-x", type=int, default=DEFAULTS["cells_x"], help="Cells X")
    parser.add_argument("--cells-y", type=int, default=DEFAULTS["cells_y"], help="Cells Y")
    parser.add_argument("--ssdv", action="store_true", help="Encode with SSDV after capture")

    args = parser.parse_args()

    if args.image_id is None:
        main_interactive()
    else:
        main_cli(args)


if __name__ == "__main__":
    main()
