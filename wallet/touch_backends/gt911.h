#pragma once

#include <Arduino.h>
#include <Wire.h>

#include "../board_profile.h"
#include "../touch_transform.h"

// Minimal GT911 transport adapter. It owns only controller discovery and raw
// point acquisition; debouncing, button mapping, and authorization policy stay
// in the common input layer.
class BowserGt911 {
 public:
  bool begin(const Gt911Profile &profile) {
    if (!profile.enabled) return false;

    Wire.begin(profile.sdaPin, profile.clockPin, 400000);
    pinMode(profile.resetPin, OUTPUT);
    pinMode(profile.interruptPin, OUTPUT);

    const uint8_t addresses[2] = {0x5D, 0x14};
    for (uint8_t interruptLevel = LOW; interruptLevel <= HIGH; interruptLevel++) {
      digitalWrite(profile.interruptPin, interruptLevel);
      digitalWrite(profile.resetPin, LOW);
      delay(12);
      digitalWrite(profile.resetPin, HIGH);
      delay(60);

      for (uint8_t address : addresses) {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0) {
          address_ = address;
          break;
        }
      }
      if (address_ != 0) break;
    }

    pinMode(profile.interruptPin, INPUT);
    if (address_ == 0) {
      Wire.end();
      return false;
    }

    // Confirm that the responding device exposes the GT911 product-ID
    // register before enabling it as a wallet input.
    uint8_t productId[4] = {0};
    if (
      !readRegisters(0x8140, productId, sizeof(productId)) ||
      productId[0] != '9' ||
      productId[1] != '1' ||
      productId[2] != '1'
    ) {
      address_ = 0;
      Wire.end();
      return false;
    }
    return true;
  }

  bool poll(
    const Gt911Profile &profile,
    int16_t displayWidth,
    int16_t displayHeight,
    uint16_t &screenX,
    uint16_t &screenY
  ) {
    uint8_t report[9] = {0};
    if (!readRegisters(0x814E, report, sizeof(report))) {
      touching_ = false;
      return false;
    }

    if ((report[0] & 0x80) == 0) return touching_;

    uint8_t pointCount = report[0] & 0x0F;
    // A point is not accepted unless the controller also accepts the report
    // acknowledgement. Otherwise a stale ready report could be replayed.
    if (!writeRegister(0x814E, 0)) {
      touching_ = false;
      return false;
    }

    // The wallet UI is deliberately single-touch. Reject an ambiguous
    // multi-touch report rather than allowing controller ordering to choose
    // which on-screen action is taken.
    if (pointCount != 1) {
      touching_ = false;
      return false;
    }

    uint16_t rawX = uint16_t(report[2]) | (uint16_t(report[3]) << 8);
    uint16_t rawY = uint16_t(report[4]) | (uint16_t(report[5]) << 8);
    if (
      !bowserMapTouchPoint(
        rawX,
        rawY,
        profile.rawWidth,
        profile.rawHeight,
        profile.swapAxes,
        profile.invertX,
        profile.invertY,
        displayWidth,
        displayHeight,
        x_,
        y_
      )
    ) {
      touching_ = false;
      return false;
    }
    screenX = x_;
    screenY = y_;
    touching_ = true;
    return true;
  }

  bool touching(uint16_t &screenX, uint16_t &screenY) const {
    if (!touching_) return false;
    screenX = x_;
    screenY = y_;
    return true;
  }

 private:
  bool readRegisters(uint16_t reg, uint8_t *data, size_t length) {
    if (address_ == 0 || length > 255) return false;

    Wire.beginTransmission(address_);
    Wire.write(uint8_t(reg >> 8));
    Wire.write(uint8_t(reg));
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom(address_, uint8_t(length)) != length) return false;
    for (size_t index = 0; index < length; index++) data[index] = Wire.read();
    return true;
  }

  bool writeRegister(uint16_t reg, uint8_t value) {
    if (address_ == 0) return false;

    Wire.beginTransmission(address_);
    Wire.write(uint8_t(reg >> 8));
    Wire.write(uint8_t(reg));
    Wire.write(value);
    return Wire.endTransmission() == 0;
  }

  uint8_t address_ = 0;
  bool touching_ = false;
  uint16_t x_ = 0;
  uint16_t y_ = 0;
};
