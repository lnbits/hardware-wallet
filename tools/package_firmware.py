#!/usr/bin/env python3
"""Package one Bowser board build for ESP Web Tools and release assets."""

from __future__ import annotations

import argparse
import json
import shutil
import tempfile
from pathlib import Path

from esp_image_hash import esp_image_hash


TARGETS = {
    "lilygo_tdisplay": {"chip_family": "ESP32", "bootloader_offset": 0x1000},
    "esp32_2432s028r": {"chip_family": "ESP32", "bootloader_offset": 0x1000},
    "esp32_3248s035r": {"chip_family": "ESP32", "bootloader_offset": 0x1000},
    "esp32_3248s035c": {"chip_family": "ESP32", "bootloader_offset": 0x1000},
    "waveshare_esp32_c6_lcd_1_3": {
        "chip_family": "ESP32-C6",
        "bootloader_offset": 0x0,
    },
    "lilygo_tdisplay_s3_amoled": {
        "chip_family": "ESP32-S3",
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


def write_package(args: argparse.Namespace, output_directory: Path) -> str:
    target = TARGETS[args.target]
    application_source = args.build_directory / "wallet.ino.bin"
    application_destination = output_directory / "wallet.ino.bin"
    copy_required(application_source, application_destination)
    copy_required(
        args.build_directory / "wallet.ino.bootloader.bin",
        output_directory / "bootloader.bin",
    )
    copy_required(
        args.build_directory / "wallet.ino.partitions.bin",
        output_directory / "wallet.ino.partitions.bin",
    )

    parts = [
        {"path": "bootloader.bin", "offset": target["bootloader_offset"]},
        {"path": "wallet.ino.partitions.bin", "offset": 0x8000},
    ]
    if args.boot_app0:
        copy_required(args.boot_app0, output_directory / "boot_app0.bin")
        parts.append({"path": "boot_app0.bin", "offset": 0xE000})
    parts.append({"path": "wallet.ino.bin", "offset": 0x10000})

    firmware_hash = esp_image_hash(application_destination)
    if firmware_hash != esp_image_hash(application_source):
        raise ValueError("packaged firmware fingerprint differs from build output")
    (output_directory / "ESP_IMAGE_SHA256.txt").write_text(
        firmware_hash + "\n", encoding="ascii"
    )
    manifest = {
        "name": f"Bowser Wallet — {args.target}",
        "version": args.version,
        "funding_url": "https://github.com/lnbits/hardware-wallet",
        "new_install_prompt_erase": True,
        "new_install_improv_wait_time": 0,
        "builds": [{"chipFamily": target["chip_family"], "parts": parts}],
    }
    (output_directory / "manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="utf-8"
    )
    return firmware_hash


def replace_package(output_directory: Path, staging_directory: Path) -> None:
    if output_directory.is_symlink():
        raise ValueError(f"refusing to replace symlink: {output_directory}")
    if output_directory.exists() and not output_directory.is_dir():
        raise ValueError(f"package destination is not a directory: {output_directory}")

    backup_directory = staging_directory.with_name(staging_directory.name + ".old")
    had_previous_package = output_directory.exists()
    if had_previous_package:
        output_directory.rename(backup_directory)

    try:
        staging_directory.rename(output_directory)
    except Exception:
        if had_previous_package:
            backup_directory.rename(output_directory)
        raise
    else:
        if had_previous_package:
            shutil.rmtree(backup_directory)


def main() -> int:
    args = parse_args()
    # Keep the final path itself unresolved so the symlink guard in
    # replace_package() cannot be bypassed by Path.resolve().
    output_directory = args.output_directory.absolute()
    output_directory.parent.mkdir(parents=True, exist_ok=True)
    staging_directory = Path(
        tempfile.mkdtemp(
            prefix=f".{output_directory.name}.new-", dir=output_directory.parent
        )
    )

    try:
        firmware_hash = write_package(args, staging_directory)
        replace_package(output_directory, staging_directory)
    finally:
        if staging_directory.exists():
            shutil.rmtree(staging_directory)

    print(f"Packaged {args.target}: {firmware_hash}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
