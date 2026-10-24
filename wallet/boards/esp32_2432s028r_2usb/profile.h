#pragma once

// ESP32-2432S028R with micro USB and USB-C ports. Wiring matches
// esp32_2432s028r; only the display controller differs:
// https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/blob/main/cyd.md
// https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/blob/main/PINS.md

const BoardProfile BOARD = {
  "esp32_2432s028r_2usb",
  "ESP32-2432S028R (2 USB)",
  1,
  false,
  1,
  21,
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
  {true, 25, 39, 32, 33, 36, false},
  {false, -1, -1, -1, -1, 0, 0, false, false, false},
};

inline SPIClass &boardSdSpi() {
  return SPI;
}
