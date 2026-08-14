#pragma once

// Board definition reference:
// https://github.com/rzeldent/platformio-espressif32-sunton/blob/main/esp32-3248S035C.json

const BoardProfile BOARD = {
  "esp32_3248s035c",
  "ESP32-3248S035C",
  1,
  false,
  1,
  27,
  HIGH,
  0,
  -1,
  false,
  false,
  {-1, -1, -1},
  {-1, -1, -1, -1},
  true,
  18,
  19,
  23,
  5,
  true,
  {false, -1, -1, -1, -1, -1, false},
  // Native GT911 coordinates are portrait 320x480. Rotation 1 maps them to
  // the wallet's 480x320 landscape UI.
  {true, 33, 32, 25, 21, 320, 480, true, false, true},
};

inline SPIClass &boardSdSpi() {
  return SPI;
}
