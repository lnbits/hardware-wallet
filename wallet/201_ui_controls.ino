namespace {
const int MAX_UI_TOUCH_BUTTONS = 8;
UiTouchButton uiTouchButtons[MAX_UI_TOUCH_BUTTONS];
uint8_t uiTouchButtonCount = 0;
int16_t currentUiContentHeight = 0;

int16_t controlsHeight(UiControls controls) {
  if (!BOARD.touch.enabled || controls == UiControls::None) return 0;
  int16_t available = tft.height();
  if (controls == UiControls::DicePad) {
    int16_t half = available / 2;
    return half < 104 ? half : 104;
  }
  int16_t third = available / 3;
  return third < 58 ? third : 58;
}

void addTouchButton(
  UiRect bounds,
  const String &label,
  char key,
  uint16_t color,
  bool enabled = true
) {
  if (uiTouchButtonCount >= MAX_UI_TOUCH_BUTTONS) return;

  UiTouchButton &button = uiTouchButtons[uiTouchButtonCount++];
  button.bounds = bounds;
  button.key = key;
  button.enabled = enabled;

  uint16_t fill = enabled ? color : TFT_DARKGREY;
  tft.fillRect(bounds.x + 2, bounds.y + 2, bounds.width - 4, bounds.height - 4, fill);
  tft.drawRect(bounds.x + 1, bounds.y + 1, bounds.width - 2, bounds.height - 2, TFT_WHITE);
  tft.setTextColor(TFT_WHITE, fill);
  tft.setTextSize(2);
  int16_t textWidth = label.length() * 12;
  int16_t centeredX = (bounds.width - textWidth) / 2;
  int16_t centeredY = (bounds.height - 16) / 2;
  int16_t textX = bounds.x + (centeredX > 4 ? centeredX : 4);
  int16_t textY = bounds.y + (centeredY > 4 ? centeredY : 4);
  tft.setCursor(textX, textY);
  tft.print(label);
}

void drawTwoButtonRow(
  int16_t top,
  int16_t height,
  const String &leftLabel,
  char leftKey,
  const String &rightLabel,
  char rightKey,
  bool rightEnabled
) {
  int16_t halfWidth = tft.width() / 2;
  addTouchButton({0, top, halfWidth, height}, leftLabel, leftKey, TFT_RED);
  addTouchButton(
    {halfWidth, top, int16_t(tft.width() - halfWidth), height},
    rightLabel,
    rightKey,
    TFT_DARKGREEN,
    rightEnabled
  );
}
}

void clearUiTouchButtons() {
  uiTouchButtonCount = 0;
}

int16_t uiContentHeight() {
  return currentUiContentHeight > 0 ? currentUiContentHeight : tft.height();
}

void beginUiScreen(UiControls controls, bool confirmEnabled) {
  clearUiTouchButtons();
  currentUiContentHeight = tft.height() - controlsHeight(controls);
  tft.fillScreen(TFT_BLACK);
  setUiControls(controls, confirmEnabled);
}

void setUiControls(UiControls controls, bool confirmEnabled) {
  clearUiTouchButtons();
  int16_t footerHeight = controlsHeight(controls);
  currentUiContentHeight = tft.height() - footerHeight;
  if (footerHeight == 0) return;

  int16_t top = tft.height() - footerHeight;
  tft.fillRect(0, top, tft.width(), footerHeight, TFT_BLACK);

  if (controls == UiControls::DicePad) {
    int16_t rowHeight = footerHeight / 2;
    int16_t columnWidth = tft.width() / 4;
    const char topKeys[4] = {'1', '2', '3', '*'};
    const char bottomKeys[4] = {'4', '5', '6', '#'};
    const char *topLabels[4] = {"1", "2", "3", "Del"};
    const char *bottomLabels[4] = {"4", "5", "6", "Done"};
    for (int column = 0; column < 4; column++) {
      int16_t x = column * columnWidth;
      int16_t width = column == 3 ? tft.width() - x : columnWidth;
      addTouchButton(
        {x, top, width, rowHeight},
        topLabels[column],
        topKeys[column],
        column == 3 ? TFT_RED : TFT_NAVY
      );
      addTouchButton(
        {x, int16_t(top + rowHeight), width, int16_t(footerHeight - rowHeight)},
        bottomLabels[column],
        bottomKeys[column],
        column == 3 ? TFT_DARKGREEN : TFT_NAVY,
        column != 3 || confirmEnabled
      );
    }
    return;
  }

  if (controls == UiControls::ConfirmCancel) {
    drawTwoButtonRow(top, footerHeight, "Cancel", '*', "Accept", '#', confirmEnabled);
  } else if (controls == UiControls::PreviousNext) {
    drawTwoButtonRow(top, footerHeight, "Back", '*', "Next", '#', confirmEnabled);
  } else if (controls == UiControls::ContinueOnly) {
    addTouchButton({0, top, tft.width(), footerHeight}, "Continue", '#', TFT_DARKGREEN, confirmEnabled);
  } else if (controls == UiControls::ToggleOnly) {
    addTouchButton({0, top, tft.width(), footerHeight}, "Text / QR", '#', TFT_NAVY);
  }
}

char keyAtTouchPoint(uint16_t x, uint16_t y) {
  for (uint8_t index = 0; index < uiTouchButtonCount; index++) {
    const UiTouchButton &button = uiTouchButtons[index];
    if (!button.enabled) continue;
    if (
      x >= button.bounds.x &&
      y >= button.bounds.y &&
      x < button.bounds.x + button.bounds.width &&
      y < button.bounds.y + button.bounds.height
    ) {
      return button.key;
    }
  }
  return 0;
}
