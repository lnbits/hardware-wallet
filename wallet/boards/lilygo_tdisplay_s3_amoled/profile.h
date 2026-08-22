#pragma once

// Board resources and schematic:
// https://github.com/Xinyuan-LilyGO/T-Display-S3-AMOLED

const BoardProfile BOARD = {
  "lilygo_tdisplay_s3_amoled",
  "LILYGO T-Display S3 AMOLED",
  1,     // landscape: 536 wide x 240 tall
  false, // AMOLED — no inversion needed
  1,     // uiTextSizeBoost: larger screen than original T-Display
  38,    // display power enable pin
  HIGH,
  0,     // BOOT button (active low, pulled up)
  21,    // user button (active low, pulled up)
  false, // two buttons — no long-press-cancels needed
  false,
  {-1, -1, -1},
  {-1, -1, -1, -1},
  false, // no built-in SD card
  -1,
  -1,
  -1,
  -1,
  false,
  {false, -1, -1, -1, -1, -1, false},
  {false, -1, -1, -1, -1, 0, 0, false, false, false},
};

inline SPIClass &boardSdSpi() {
  static SPIClass sdSpi(HSPI);
  return sdSpi;
}
