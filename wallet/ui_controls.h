#pragma once

#include <Arduino.h>

enum class UiControls : uint8_t {
  None,
  ContinueOnly,
  ConfirmCancel,
  PreviousNext,
  ToggleOnly,
  DicePad,
};

struct UiRect {
  int16_t x;
  int16_t y;
  int16_t width;
  int16_t height;
};

struct UiTouchButton {
  UiRect bounds;
  char key;
  bool enabled;
};

void beginUiScreen(UiControls controls = UiControls::None, bool confirmEnabled = true);
void setUiControls(UiControls controls, bool confirmEnabled = true);
int16_t uiContentHeight();
uint32_t uiScreenRevision();
char keyAtTouchPoint(uint16_t x, uint16_t y);
void clearUiTouchButtons();
