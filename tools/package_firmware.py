#!/usr/bin/env python3
"""Package one Bowser board build for ESP Web Tools and release assets."""

from __future__ import annotations

import argparse
import json
import shutil
from pathlib import Path

from esp_image_hash import esp_image_hash


TARGETS = {
    "lilygo_tdisplay": {"chip_family": "ESP32", "bootloader_offset": 0x1000},
    "esp32_2432s028r": {"chip_family": "ESP32", "bootloader_offset": 0x1000},
    "waveshare_esp32_c6_lcd_1_3": {
        "chip_family": "ESP32-C6",
        "bootloader_offset": 0x0,
    },
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("target", choices=TARGETS)
    parser.add_argument("version")
    parser.add_argument("build_directory", type=Path)
    parser.add_argument("output_directory", type=Path)
    parser.add_argument("--boot-app0", type=Path)
    return parser.parse_args()


def copy_required(source: Path, destination: Path) -> None:
    if not source.is_file():
        raise FileNotFoundError(source)
    shutil.copy2(source, destination)


def main() -> int:
    args = parse_args()
    target = TARGETS[args.target]
    args.output_directory.mkdir(parents=True, exist_ok=True)

    application_source = args.build_directory / "wallet.ino.bin"
    application_destination = args.output_directory / "wallet.ino.bin"
    copy_required(application_source, application_destination)
    copy_required(
        args.build_directory / "wallet.ino.bootloader.bin",
        args.output_directory / "bootloader.bin",
    )
    copy_required(
        args.build_directory / "wallet.ino.partitions.bin",
        args.output_directory / "wallet.ino.partitions.bin",
    )

    parts = [
        {"path": "bootloader.bin", "offset": target["bootloader_offset"]},
        {"path": "wallet.ino.partitions.bin", "offset": 0x8000},
    ]
    if args.boot_app0:
        copy_required(args.boot_app0, args.output_directory / "boot_app0.bin")
        parts.append({"path": "boot_app0.bin", "offset": 0xE000})
    parts.append({"path": "wallet.ino.bin", "offset": 0x10000})

    firmware_hash = esp_image_hash(application_destination)
    (args.output_directory / "ESP_IMAGE_SHA256.txt").write_text(
        firmware_hash + "\n", encoding="ascii"
    )
    manifest = {
        "name": f"Bowser HWW — {args.target}",
        "version": args.version,
        "funding_url": "https://github.com/lnbits/hardware-wallet",
        "new_install_prompt_erase": True,
        "new_install_improv_wait_time": 0,
        "builds": [{"chipFamily": target["chip_family"], "parts": parts}],
    }
    (args.output_directory / "manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="utf-8"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
