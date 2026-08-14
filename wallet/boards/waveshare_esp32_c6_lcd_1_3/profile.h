#pragma once

// Board resources and schematic:
// https://docs.waveshare.com/ESP32-C6-LCD-1.3

const BoardProfile BOARD = {
  "waveshare_esp32_c6_lcd_1_3",
  "Waveshare ESP32-C6-LCD-1.3",
  0,
  true,
  0,
  22,
  HIGH,
  9,
  -1,
  true,
  false,
  {-1, -1, -1},
  {-1, -1, -1, -1},
  true,
  7,
  5,
  6,
  4,
  false,
  {false, -1, -1, -1, -1, -1, false},
  {false, -1, -1, -1, -1, 0, 0, false, false, false},
};

inline SPIClass &boardSdSpi() {
  return SPI;
}
