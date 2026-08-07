#!/usr/bin/env python3
"""Print and verify the SHA-256 embedded in an ESP application image."""

from __future__ import annotations

import hashlib
import sys
from pathlib import Path


def esp_image_hash(path: Path) -> str:
    image = path.read_bytes()
    if len(image) < 56 or image[0] != 0xE9:
        raise ValueError(f"{path} is not an ESP application image")
    if image[23] != 1:
        raise ValueError(f"{path} does not contain an appended SHA-256")

    content, embedded_digest = image[:-32], image[-32:]
    calculated_digest = hashlib.sha256(content).digest()
    if calculated_digest != embedded_digest:
        raise ValueError(f"{path} has an invalid appended SHA-256")
    return embedded_digest.hex()


def main() -> int:
    if len(sys.argv) != 2:
        print(f"Usage: {Path(sys.argv[0]).name} <ESP-application.bin>", file=sys.stderr)
        return 2
    try:
        print(esp_image_hash(Path(sys.argv[1])))
    except (OSError, ValueError) as error:
        print(error, file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
