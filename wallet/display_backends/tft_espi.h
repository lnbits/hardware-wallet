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
