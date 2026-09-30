#pragma once

// Board resources and schematic:
// https://github.com/Xinyuan-LilyGO/T-Display-S3-AMOLED

const BoardProfile BOARD = {
  "lilygo_tdisplay_s3_amoled",
  "LILYGO T-Display S3 AMOLED",
  3,     // landscape (flipped): 536 wide x 240 tall
  false, // AMOLED — no inversion needed
  1,     // uiTextSizeBoost: larger screen than original T-Display
  38,    // display power enable pin
  HIGH,
  0,     // BOOT button (active low, pulled up)
  21,    // user button (active low, pulled up)
  false, // two buttons — no long-press-cancels needed
  true,  // enable keypad
  {11, 12, 13},  // C1, C2, C3
  {1, 2, 3, 10}, // R1, R2, R3, R4
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
