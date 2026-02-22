#!/usr/bin/env python3
"""
Simple CLI to take a video with optional custom parameters.
Run from the payload directory: python take_vid_cli.py

Interactive mode (no args):
    python take_vid_cli.py

CLI args mode:
    python take_vid_cli.py VIDEO_ID [--camera-num 0] [options...]
"""

import argparse

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


def run_vid(vid_id: str, camera_num: int, camera_name: str,
            libcamera_config: str, ffmpeg_in_config: str, ffmpeg_out_config: str):
    """Take video and return result tuple."""
    result = commands.take_vid(
        vid_id,
        camera_num=camera_num,
        camera_name=camera_name,
        libcamera_config=libcamera_config,
        ffmpeg_in_config=ffmpeg_in_config,
        ffmpeg_out_config=ffmpeg_out_config,
    )
    raw_size, compressed_size = result
    print(f"\nRaw size: {raw_size} bytes")
    print(f"Compressed size: {compressed_size} bytes")
    return raw_size, compressed_size


def main_interactive():
    """Interactive mode: prompt for all inputs."""
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
    else:
        camera_num = DEFAULTS["camera_num"]
        camera_name = DEFAULTS["camera_name"]
        libcamera_config = DEFAULTS["libcamera_config"]
        ffmpeg_in_config = DEFAULTS["ffmpeg_in_config"]
        ffmpeg_out_config = DEFAULTS["ffmpeg_out_config"]

    run_vid(vid_id, camera_num, camera_name, libcamera_config, ffmpeg_in_config, ffmpeg_out_config)


def main_cli(args: argparse.Namespace):
    """CLI args mode: use parsed arguments."""
    run_vid(
        args.vid_id,
        args.camera_num,
        args.camera_name,
        args.libcamera_config,
        args.ffmpeg_in_config,
        args.ffmpeg_out_config,
    )


def main():
    parser = argparse.ArgumentParser(
        description="Take a video with optional custom parameters.",
        epilog="Run with no arguments for interactive mode.",
    )
    parser.add_argument(
        "vid_id",
        nargs="?",
        help="Video ID (omit for interactive mode)",
    )
    parser.add_argument("--camera-num", type=int, default=DEFAULTS["camera_num"], help="Camera number")
    parser.add_argument("--camera-name", default=DEFAULTS["camera_name"], help="Camera name")
    parser.add_argument(
        "--libcamera-config",
        default=DEFAULTS["libcamera_config"],
        help="Libcamera config (default=5s, longvid=30s)",
    )
    parser.add_argument(
        "--ffmpeg-in-config",
        default=DEFAULTS["ffmpeg_in_config"],
        help="FFmpeg input config",
    )
    parser.add_argument(
        "--ffmpeg-out-config",
        default=DEFAULTS["ffmpeg_out_config"],
        help="FFmpeg output config",
    )

    args = parser.parse_args()

    if args.vid_id is None:
        main_interactive()
    else:
        main_cli(args)


if __name__ == "__main__":
    main()
