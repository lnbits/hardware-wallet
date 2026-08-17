"""Encode the Bowser banner as an RGB565 Arduino PROGMEM bitmap."""

from pathlib import Path

from PIL import Image


PROJECT_ROOT = Path(__file__).resolve().parent.parent
SOURCE = PROJECT_ROOT / "assets" / "bowser.png"
OUTPUT = PROJECT_ROOT / "wallet" / "bowser_logo.h"
WIDTH = 240
HEIGHT = 60


def rgb565(red: int, green: int, blue: int) -> int:
    return ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3)


source = Image.open(SOURCE).convert("RGBA")
background = Image.new("RGBA", source.size, (0, 0, 0, 255))
background.alpha_composite(source)
image = background.convert("RGB").resize(
    (WIDTH, HEIGHT),
    resample=Image.Resampling.NEAREST,
)
raw_pixels = image.tobytes()
pixels = [
    rgb565(*raw_pixels[offset : offset + 3])
    for offset in range(0, len(raw_pixels), 3)
]

lines = [
    "#pragma once",
    "",
    "#include <Arduino.h>",
    "",
    "// Generated from assets/bowser.png by tools/encode_banner.py.",
    f"const uint16_t BOWSER_LOGO_WIDTH = {WIDTH};",
    f"const uint16_t BOWSER_LOGO_HEIGHT = {HEIGHT};",
    f"const uint16_t BOWSER_LOGO_PIXELS[{len(pixels)}] PROGMEM = {{",
]
for offset in range(0, len(pixels), 12):
    chunk = ", ".join(f"0x{value:04X}" for value in pixels[offset : offset + 12])
    lines.append(f"  {chunk},")
lines.extend(["};", ""])

OUTPUT.write_text("\n".join(lines), encoding="utf-8")
print(f"Encoded {SOURCE.name} as {WIDTH}x{HEIGHT} RGB565 ({len(pixels) * 2} bytes)")
