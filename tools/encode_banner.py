"""Encode Bowser UI artwork as RGB565 Arduino PROGMEM bitmaps."""

from pathlib import Path

from PIL import Image


PROJECT_ROOT = Path(__file__).resolve().parent.parent
LOGO_SOURCE = PROJECT_ROOT / "assets" / "bowser.png"
LOGO_OUTPUT = PROJECT_ROOT / "wallet" / "bowser_logo.h"
LOGO_WIDTH = 240
LOGO_HEIGHT = 60
THINKING_SOURCES = [
    PROJECT_ROOT / "assets" / f"thinking{frame}.png"
    for frame in range(1, 4)
]
THINKING_OUTPUT = PROJECT_ROOT / "wallet" / "thinking_animation.h"
THINKING_WIDTH = 192
THINKING_HEIGHT = 192
# All three source frames share this square canvas. Trimming only the empty
# outer border keeps the enlarged artwork readable on every display.
THINKING_CROP = (67, 36, 361, 330)


def rgb565(red: int, green: int, blue: int) -> int:
    return ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3)


def encode_pixels(
    source_path: Path,
    width: int,
    height: int,
    crop_box: tuple[int, int, int, int] | None = None,
) -> list[int]:
    source = Image.open(source_path).convert("RGBA")
    if crop_box is not None:
        source = source.crop(crop_box)
    background = Image.new("RGBA", source.size, (0, 0, 0, 255))
    background.alpha_composite(source)
    image = background.convert("RGB").resize(
        (width, height),
        resample=Image.Resampling.NEAREST,
    )
    raw_pixels = image.tobytes()
    return [
        rgb565(*raw_pixels[offset : offset + 3])
        for offset in range(0, len(raw_pixels), 3)
    ]


def append_pixels(lines: list[str], pixels: list[int], indent: str = "  ") -> None:
    for offset in range(0, len(pixels), 12):
        chunk = ", ".join(
            f"0x{value:04X}" for value in pixels[offset : offset + 12]
        )
        lines.append(f"{indent}{chunk},")


def encode_logo() -> None:
    pixels = encode_pixels(LOGO_SOURCE, LOGO_WIDTH, LOGO_HEIGHT)
    lines = [
        "#pragma once",
        "",
        "#include <Arduino.h>",
        "",
        "// Generated from assets/bowser.png by tools/encode_banner.py.",
        f"const uint16_t BOWSER_LOGO_WIDTH = {LOGO_WIDTH};",
        f"const uint16_t BOWSER_LOGO_HEIGHT = {LOGO_HEIGHT};",
        f"const uint16_t BOWSER_LOGO_PIXELS[{len(pixels)}] PROGMEM = {{",
    ]
    append_pixels(lines, pixels)
    lines.extend(["};", ""])
    LOGO_OUTPUT.write_text("\n".join(lines), encoding="utf-8")
    print(
        f"Encoded {LOGO_SOURCE.name} as {LOGO_WIDTH}x{LOGO_HEIGHT} RGB565 "
        f"({len(pixels) * 2} bytes)"
    )


def encode_thinking_animation() -> None:
    frames = [
        encode_pixels(
            source,
            THINKING_WIDTH,
            THINKING_HEIGHT,
            THINKING_CROP,
        )
        for source in THINKING_SOURCES
    ]
    pixels_per_frame = THINKING_WIDTH * THINKING_HEIGHT
    lines = [
        "#pragma once",
        "",
        "#include <Arduino.h>",
        "",
        "// Generated from assets/thinking1.png through thinking3.png by tools/encode_banner.py.",
        f"const uint16_t THINKING_FRAME_WIDTH = {THINKING_WIDTH};",
        f"const uint16_t THINKING_FRAME_HEIGHT = {THINKING_HEIGHT};",
        f"const uint8_t THINKING_FRAME_COUNT = {len(frames)};",
        "const uint16_t THINKING_FRAMES[THINKING_FRAME_COUNT]"
        f"[{pixels_per_frame}] PROGMEM = {{",
    ]
    for frame in frames:
        lines.append("  {")
        append_pixels(lines, frame, "    ")
        lines.append("  },")
    lines.extend(["};", ""])
    THINKING_OUTPUT.write_text("\n".join(lines), encoding="utf-8")
    print(
        f"Encoded {len(frames)} thinking frames as "
        f"{THINKING_WIDTH}x{THINKING_HEIGHT} RGB565 "
        f"({len(frames) * pixels_per_frame * 2} bytes)"
    )


if __name__ == "__main__":
    encode_logo()
    encode_thinking_animation()
