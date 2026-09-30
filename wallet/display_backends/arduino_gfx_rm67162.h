#pragma once

// RM67162 AMOLED display backend via Arduino_GFX.
//
// Exposes a TFT_eSPI-compatible surface (BowserDisplay) so application code
// needs no changes. Only the methods actually called by the wallet are bridged;
// the rest are inherited from Arduino_GFX / Arduino_RM67162.
//
// T-Display S3 AMOLED QSPI wiring (from LilyGO schematic):
//   CS=6  CLK=47  D0=18  D1=7  D2=48  D3=5  RST=17  TE=9(unused)

#include <Arduino_GFX.h>
#include <databus/Arduino_ESP32QSPI.h>
#include <display/Arduino_RM67162.h>

// TFT_eSPI colour constants used by wallet UI code.
#ifndef TFT_BLACK
#define TFT_BLACK   0x0000
#define TFT_WHITE   0xFFFF
#define TFT_RED     0xF800
#define TFT_GREEN   0x07E0
#define TFT_BLUE    0x001F
#define TFT_YELLOW  0xFFE0
#define TFT_CYAN    0x07FF
#define TFT_MAGENTA 0xF81F
#define TFT_ORANGE  0xFDA0
#define TFT_PURPLE  0x780F
#define TFT_PINK    0xFE19
#define TFT_GREY    0x7BEF
#define TFT_DARKGREY 0x7BEF
#define TFT_DARKCYAN 0x03EF
#define TFT_OLIVE   0x7BE0
#define TFT_MAROON  0x7800
#define TFT_NAVY    0x000F
#define TFT_DARKGREEN 0x03E0
#endif

class BowserDisplay : public Arduino_RM67162 {
  static Arduino_ESP32QSPI &bus() {
    static Arduino_ESP32QSPI instance(
      6,   // CS
      47,  // CLK
      18,  // D0
      7,   // D1
      48,  // D2
      5    // D3
    );
    return instance;
  }

  bool _swapBytes = false;

public:
  BowserDisplay() : Arduino_RM67162(&bus(), 17 /* RST */) {}

  // TFT_eSPI uses init(); Arduino_GFX uses begin().
  void init() { Arduino_RM67162::begin(); }

  // TFT_eSPI byte-swap flag: when true, each uint16_t pixel's bytes are
  // swapped before sending. Used by pushImage for logo/animation data stored
  // in big-endian RGB565 format.
  void setSwapBytes(bool swap) { _swapBytes = swap; }
  bool getSwapBytes() const { return _swapBytes; }

  // Push a block of RGB565 pixels. Respects the swapBytes flag.
  void pushImage(int32_t x, int32_t y, int32_t w, int32_t h,
                 const uint16_t *data) {
    uint32_t len = (uint32_t)w * h;
    startWrite();
    writeAddrWindow(x, y, w, h);
    if (!_swapBytes) {
      writePixels(const_cast<uint16_t *>(data), len);
    } else {
      for (uint32_t i = 0; i < len; i++) {
        uint16_t px = data[i];
        uint16_t swapped = (uint16_t)((px >> 8) | (px << 8));
        writePixels(&swapped, 1);
      }
    }
    endWrite();
  }

  // Stubs for TFT_eSPI touch API — unused on this board (hasTouchscreen=false)
  // but referenced in compiled touch paths that are excluded at runtime.
  bool getTouchRaw(uint16_t *x, uint16_t *y) { (void)x; (void)y; return false; }
  uint8_t getTouchRawZ() { return 0; }
};

inline void beginBowserDisplay(BowserDisplay &display) {
  // GPIO 38 gates the AMOLED boost converter on the T-Display S3 AMOLED.
  // It must be driven HIGH before the QSPI init sequence or the controller
  // gets no power and all commands are lost.
  pinMode(38, OUTPUT);
  digitalWrite(38, HIGH);
  delay(20);
  display.init();
}
