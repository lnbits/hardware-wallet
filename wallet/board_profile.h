#pragma once

#include <Arduino.h>
#include <SPI.h>

struct TouchProfile {
  bool enabled;
  int8_t clockPin;
  int8_t misoPin;
  int8_t mosiPin;
  int8_t chipSelectPin;
  int8_t interruptPin;
};

struct BoardProfile {
  const char *id;
  const char *displayName;
  uint8_t displayRotation;
  bool invertDisplay;
  int8_t backlightPin;
  uint8_t backlightOnLevel;
  int8_t primaryButtonPin;
  int8_t secondaryButtonPin;
  bool singleButtonLongPressCancels;
  bool hasMatrixKeypad;
  int8_t keypadColumnPins[3];
  int8_t keypadRowPins[4];
  bool hasSdCard;
  int8_t sdClockPin;
  int8_t sdMisoPin;
  int8_t sdMosiPin;
  int8_t sdChipSelectPin;
  TouchProfile touch;
};

#define BOWSER_STRINGIFY_IMPL(value) #value
#define BOWSER_STRINGIFY(value) BOWSER_STRINGIFY_IMPL(value)

#ifndef BOWSER_BOARD_PROFILE
#define BOWSER_BOARD_PROFILE boards/lilygo_tdisplay/profile.h
#endif

#include BOWSER_STRINGIFY(BOWSER_BOARD_PROFILE)
