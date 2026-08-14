#pragma once

const BoardProfile BOARD = {
  "lilygo_tdisplay",
  "LILYGO T-Display",
  1,
  true,
  0,
  4,
  HIGH,
  0,
  35,
  false,
  true,
  {33, 32, 25},
  {21, 27, 26, 22},
  true,
  17,
  2,
  15,
  13,
  false,
  {false, -1, -1, -1, -1, -1, false},
  {false, -1, -1, -1, -1, 0, 0, false, false, false},
};

inline SPIClass &boardSdSpi() {
  static SPIClass sdSpi(HSPI);
  return sdSpi;
}
