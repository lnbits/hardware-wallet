from __future__ import annotations

import hashlib
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
PACKAGER = REPOSITORY_ROOT / "tools" / "package_firmware.py"


def valid_esp_application() -> bytes:
    content = bytearray(64)
    content[0] = 0xE9
    content[23] = 1
    return bytes(content) + hashlib.sha256(content).digest()


class PackageFirmwareTest(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary_directory = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary_directory.name)
        self.build_directory = self.root / "build"
        self.output_directory = self.root / "package"
        self.build_directory.mkdir()
        (self.build_directory / "wallet.ino.bin").write_bytes(
            valid_esp_application()
        )
        (self.build_directory / "wallet.ino.bootloader.bin").write_bytes(
            b"bootloader"
        )
        (self.build_directory / "wallet.ino.partitions.bin").write_bytes(
            b"partitions"
        )

    def tearDown(self) -> None:
        self.temporary_directory.cleanup()

    def package(self, *, check: bool = True) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            [
                sys.executable,
                str(PACKAGER),
                "lilygo_tdisplay",
                "test-version",
                str(self.build_directory),
                str(self.output_directory),
            ],
            check=check,
            capture_output=True,
            text=True,
        )

    def package_snapshot(self) -> dict[str, bytes]:
        return {
            path.name: path.read_bytes()
            for path in self.output_directory.iterdir()
            if path.is_file()
        }

    def test_replaces_existing_package_without_leaving_stale_files(self) -> None:
        self.output_directory.mkdir()
        (self.output_directory / "stale.bin").write_bytes(b"old build")

        result = self.package()

        self.assertNotIn("stale.bin", self.package_snapshot())
        manifest = json.loads(
            (self.output_directory / "manifest.json").read_text(encoding="utf-8")
        )
        self.assertEqual(manifest["version"], "test-version")
        expected_hash = hashlib.sha256(valid_esp_application()[:-32]).hexdigest()
        self.assertIn(expected_hash, result.stdout)
        self.assertEqual(
            (self.output_directory / "ESP_IMAGE_SHA256.txt").read_text(
                encoding="ascii"
            ),
            expected_hash + "\n",
        )

    def test_failed_package_preserves_previous_package(self) -> None:
        self.package()
        original_package = self.package_snapshot()
        (self.build_directory / "wallet.ino.partitions.bin").unlink()

        result = self.package(check=False)

        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(self.package_snapshot(), original_package)


if __name__ == "__main__":
    unittest.main()
