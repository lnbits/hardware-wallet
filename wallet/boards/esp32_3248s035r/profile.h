#pragma once

// Board definition reference:
// https://github.com/rzeldent/platformio-espressif32-sunton/blob/main/esp32-3248S035R.json

const BoardProfile BOARD = {
  "esp32_3248s035r",
  "ESP32-3248S035R",
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
  {true, 14, 12, 13, 33, 36, true},
  {false, -1, -1, -1, -1, 0, 0, false, false, false},
};

inline SPIClass &boardSdSpi() {
  return SPI;
}
