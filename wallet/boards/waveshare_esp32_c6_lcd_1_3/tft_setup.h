#pragma once

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
