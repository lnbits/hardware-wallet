#pragma once

// This board uses the arduino_gfx_rm67162 display backend, not TFT_eSPI.
// This stub satisfies TFT_eSPI's compilation requirements since the library
// is compiled as part of the build even when not used at runtime.

#define ILI9341_DRIVER
#define TFT_WIDTH  240
#define TFT_HEIGHT 536
#define TFT_MOSI   11
#define TFT_SCLK   12
#define TFT_CS     10
#define TFT_DC     13
#define TFT_RST    -1
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define SPI_FREQUENCY 40000000
