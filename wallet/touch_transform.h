#pragma once

#include <stdint.h>

// Convert a controller-native point to display coordinates. Keeping this
// transformation independent of Arduino and the touch transport makes board
// orientation profiles testable on the host.
inline bool bowserMapTouchPoint(
  uint16_t rawX,
  uint16_t rawY,
  uint16_t rawWidth,
  uint16_t rawHeight,
  bool swapAxes,
  bool invertX,
  bool invertY,
  int16_t displayWidth,
  int16_t displayHeight,
  uint16_t &screenX,
  uint16_t &screenY
) {
  uint16_t orientedX = swapAxes ? rawY : rawX;
  uint16_t orientedY = swapAxes ? rawX : rawY;
  uint16_t orientedWidth = swapAxes ? rawHeight : rawWidth;
  uint16_t orientedHeight = swapAxes ? rawWidth : rawHeight;
  if (
    orientedWidth < 2 ||
    orientedHeight < 2 ||
    displayWidth < 2 ||
    displayHeight < 2
  ) return false;

  if (orientedX >= orientedWidth) orientedX = orientedWidth - 1;
  if (orientedY >= orientedHeight) orientedY = orientedHeight - 1;
  if (invertX) orientedX = orientedWidth - 1 - orientedX;
  if (invertY) orientedY = orientedHeight - 1 - orientedY;

  screenX = uint32_t(orientedX) * (displayWidth - 1) / (orientedWidth - 1);
  screenY = uint32_t(orientedY) * (displayHeight - 1) / (orientedHeight - 1);
  return true;
}
