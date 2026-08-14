#pragma once

// TFT_eSPI 2.5.44's optimized C6 path is incompatible with ESP32 Arduino
// core 3.3.x. Select its unmodified, portable SPI backend instead. The common
// Bowser TFT_eSPI adapter restores ESP32 after this header is processed so the
// application and all non-display dependencies retain their normal target.
#if defined(CONFIG_IDF_TARGET_ESP32C6)
#undef ESP32
#endif

#define ST7789_DRIVER
#define TFT_WIDTH 240
#define TFT_HEIGHT 240

#define TFT_MISO 5
#define TFT_MOSI 6
#define TFT_SCLK 7
#define TFT_CS 14
#define TFT_DC 15
#define TFT_RST 21
#define TFT_BL 22
#define TFT_BACKLIGHT_ON HIGH

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define SPI_FREQUENCY 40000000
#define SPI_READ_FREQUENCY 6000000
