#!/usr/bin/env python3
"""
Simple CLI to take a photo with optional custom parameters.
Run from the payload directory: python take_photo_cli.py
"""

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


def main():
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
    else:
        result = commands.take_photo(image_id)

    raw_size, compressed_size, avg_cell_size, max_cell_size = result
    print(f"\nRaw size: {raw_size} bytes")
    print(f"Compressed size: {compressed_size} bytes")
    print(f"Avg cell size: {avg_cell_size} bytes")
    print(f"Max cell size: {max_cell_size} bytes")

    use_ssdv = input("Encode with SSDV? (y/N): ").strip().lower() == "y"
    if use_ssdv:
        image_path = f"{commands.IMAGES_DIR}/{image_id}.jpg"
        ssdv_path = ssdv_utils.encode_file(image_path)
        print(f"SSDV encoded to: {ssdv_path}")

if __name__ == "__main__":
    main()
