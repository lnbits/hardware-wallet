#pragma once

#if defined(ESP32) && defined(CONFIG_IDF_TARGET_ESP32C6)
#define BOWSER_RESTORE_ESP32_AFTER_TFT_ESPI
#endif

#include <TFT_eSPI.h>

#ifdef BOWSER_RESTORE_ESP32_AFTER_TFT_ESPI
#define ESP32 1
#undef BOWSER_RESTORE_ESP32_AFTER_TFT_ESPI
#endif

using BowserDisplay = TFT_eSPI;

inline void beginBowserDisplay(BowserDisplay &display) {
#ifdef BOWSER_TFT_ESPI_PREINIT_SPI
  SPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI, -1);
#endif
  display.init();
}
