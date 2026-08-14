#pragma once

#include <Arduino.h>

// TFT_eSPI's built-in bitmap font scales in whole-number steps. Profiles use
// a small additive boost so larger displays can opt into more readable text
// without changing application screens or introducing board-name branches.
inline uint8_t uiTextSize(uint8_t baseSize) {
  uint16_t scaled = uint16_t(baseSize) + BOARD.uiTextSizeBoost;
  return scaled > 7 ? 7 : uint8_t(scaled);
}

// Font 1 is six pixels wide per character at text size 1. Prefer the board's
// requested scale, but reduce it when a particular label would overflow its
// available region.
inline uint8_t uiFittedTextSize(
  const String &text,
  uint8_t baseSize,
  int16_t availableWidth
) {
  uint8_t size = uiTextSize(baseSize);
  int32_t width = availableWidth > 0 ? availableWidth : 1;
  while (size > 1 && int32_t(text.length()) * 6 * size > width) {
    size--;
  }
  return size;
}

inline int16_t uiTextPixelWidth(const String &text, uint8_t textSize) {
  return int16_t(text.length() * 6 * textSize);
}

inline int16_t uiTextPixelHeight(uint8_t textSize) {
  return int16_t(8 * textSize);
}
