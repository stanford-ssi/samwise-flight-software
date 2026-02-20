#!/usr/bin/env python3
"""
Simple CLI to take a video with optional custom parameters.
Run from the payload directory: python take_vid_cli.py
"""

import commands

DEFAULTS = {
    "camera_num": 0,
    "camera_name": "A",
    "libcamera_config": "default",
    "ffmpeg_in_config": "default",
    "ffmpeg_out_config": "default",
}

def prompt_int(prompt: str, default: int) -> int:
    raw = input(f"{prompt} [{default}]: ").strip()
    return int(raw) if raw else default

def prompt_str(prompt: str, default: str) -> str:
    raw = input(f"{prompt} [{default}]: ").strip()
    return raw if raw else default

def main():
    vid_id = input("Video ID: ").strip()
    if not vid_id:
        print("Video ID is required.")
        return

    use_custom = input("Use custom parameters? (y/N): ").strip().lower() == "y"

    if use_custom:
        camera_num = prompt_int("Camera number", DEFAULTS["camera_num"])
        camera_name = prompt_str("Camera name", DEFAULTS["camera_name"])
        libcamera_config = prompt_str(
            "Libcamera config (default=5s, longvid=30s)", DEFAULTS["libcamera_config"]
        )
        ffmpeg_in_config = prompt_str(
            "FFmpeg input config", DEFAULTS["ffmpeg_in_config"]
        )
        ffmpeg_out_config = prompt_str(
            "FFmpeg output config", DEFAULTS["ffmpeg_out_config"]
        )

        result = commands.take_vid(
            vid_id,
            camera_num=camera_num,
            camera_name=camera_name,
            libcamera_config=libcamera_config,
            ffmpeg_in_config=ffmpeg_in_config,
            ffmpeg_out_config=ffmpeg_out_config,
        )
    else:
        result = commands.take_vid(vid_id)

    raw_size, compressed_size = result
    print(f"\nRaw size: {raw_size} bytes")
    print(f"Compressed size: {compressed_size} bytes")

if __name__ == "__main__":
    main()
