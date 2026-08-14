//========================================================================//
//==========================UNIFIED PHYSICAL INPUT========================//
//========================================================================//

#include "input_debounce.h"
#include "touch_backends/gt911.h"

const char KEYPAD_KEYS[4][3] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};

namespace {
const unsigned long LONG_PRESS_CANCEL_MS = 900;
const uint32_t INPUT_DEBOUNCE_MS = 30;
const uint16_t TOUCH_PRESSURE_THRESHOLD = 350;
const char *TOUCH_CALIBRATION_FILE = "/touch.cal";
const char *TOUCH_CALIBRATION_VERSION = "2";

struct TouchCalibration {
  bool valid;
  bool screenXUsesRawX;
  int32_t xAtLeft;
  int32_t xAtRight;
  int32_t yAtTop;
  int32_t yAtBottom;
};

struct ButtonTracker {
  int8_t pin;
  bool observedDown;
  bool stableDown;
  uint32_t changedAt;
  unsigned long pressedAt;
};

TouchCalibration touchCalibration = {false, true, 0, 0, 0, 0};
ButtonTracker primaryButton = {-1, false, false, 0, 0};
ButtonTracker secondaryButton = {-1, false, false, 0, 0};
ReleasedKeyDebouncer touchKeyDebouncer;
ReleasedKeyDebouncer matrixKeyDebouncer;
bool inputHardwareInitialized = false;
BowserGt911 gt911;

uint8_t touchTransfer(uint8_t value) {
  uint8_t result = 0;
  for (int bit = 7; bit >= 0; bit--) {
    digitalWrite(BOARD.xpt2046.clockPin, LOW);
    digitalWrite(BOARD.xpt2046.mosiPin, (value >> bit) & 1);
    delayMicroseconds(1);
    digitalWrite(BOARD.xpt2046.clockPin, HIGH);
    result = (result << 1) | digitalRead(BOARD.xpt2046.misoPin);
    delayMicroseconds(1);
  }
  digitalWrite(BOARD.xpt2046.clockPin, LOW);
  return result;
}

uint16_t readTouchChannel(uint8_t command) {
  digitalWrite(BOARD.xpt2046.chipSelectPin, LOW);
  touchTransfer(command);
  uint16_t value = uint16_t(touchTransfer(0)) << 8;
  value |= touchTransfer(0);
  digitalWrite(BOARD.xpt2046.chipSelectPin, HIGH);
  return (value >> 3) & 0x0FFF;
}

uint16_t readRawTouchPressure() {
  if (!BOARD.xpt2046.enabled) return 0;

#ifdef TOUCH_CS
  if (BOARD.xpt2046.sharesDisplaySpi) {
    return tft.getTouchRawZ();
  }
#endif

  // PENIRQ is useful as a hint, but it is not reliable enough to gate input:
  // GPIOs 34-39 on the classic ESP32 have no internal pull-up, and some board
  // revisions omit or weakly populate the external pull-up. Read XPT2046
  // pressure instead so a floating/stuck IRQ cannot deadlock calibration.
  int32_t pressure = 4095;
  pressure += readTouchChannel(0xB0);  // Z1
  pressure -= readTouchChannel(0xC0);  // Z2
  if (pressure == 4095) pressure = 0;
  if (pressure < 0) return 0;
  if (pressure > 4095) return 4095;
  return uint16_t(pressure);
}

bool rawTouchIsDown() {
  if (BOARD.gt911.enabled) {
    uint16_t x = 0;
    uint16_t y = 0;
    return gt911.poll(BOARD.gt911, tft.width(), tft.height(), x, y);
  }
  return readRawTouchPressure() > TOUCH_PRESSURE_THRESHOLD;
}

void sortTouchSamples(uint16_t *samples, int count) {
  for (int index = 1; index < count; index++) {
    uint16_t value = samples[index];
    int cursor = index - 1;
    while (cursor >= 0 && samples[cursor] > value) {
      samples[cursor + 1] = samples[cursor];
      cursor--;
    }
    samples[cursor + 1] = value;
  }
}

bool readRawTouchPoint(uint16_t &rawX, uint16_t &rawY) {
  if (!rawTouchIsDown()) return false;

#ifdef TOUCH_CS
  if (BOARD.xpt2046.sharesDisplaySpi) {
    return tft.getTouchRaw(&rawX, &rawY) != 0;
  }
#endif

  const int sampleCount = 5;
  uint16_t xSamples[sampleCount];
  uint16_t ySamples[sampleCount];
  for (int sample = 0; sample < sampleCount; sample++) {
    xSamples[sample] = readTouchChannel(0xD0);
    ySamples[sample] = readTouchChannel(0x90);
  }
  sortTouchSamples(xSamples, sampleCount);
  sortTouchSamples(ySamples, sampleCount);
  rawX = xSamples[sampleCount / 2];
  rawY = ySamples[sampleCount / 2];
  return true;
}

int16_t mapTouchAxis(int32_t value, int32_t start, int32_t end, int16_t maximum) {
  if (start == end || maximum <= 0) return 0;
  int32_t mapped = (value - start) * maximum / (end - start);
  if (mapped < 0) mapped = 0;
  if (mapped > maximum) mapped = maximum;
  return int16_t(mapped);
}

bool readTouchPoint(uint16_t &x, uint16_t &y) {
  if (BOARD.gt911.enabled) return gt911.touching(x, y);

  uint16_t rawX = 0;
  uint16_t rawY = 0;
  if (!touchCalibration.valid || !readRawTouchPoint(rawX, rawY)) return false;

  int32_t screenRawX = touchCalibration.screenXUsesRawX ? rawX : rawY;
  int32_t screenRawY = touchCalibration.screenXUsesRawX ? rawY : rawX;
  x = mapTouchAxis(screenRawX, touchCalibration.xAtLeft, touchCalibration.xAtRight, tft.width() - 1);
  y = mapTouchAxis(screenRawY, touchCalibration.yAtTop, touchCalibration.yAtBottom, tft.height() - 1);
  return true;
}

bool loadTouchCalibration() {
  File calibrationFile = SPIFFS.open(TOUCH_CALIBRATION_FILE, FILE_READ);
  if (!calibrationFile) return false;

  String boardId = calibrationFile.readStringUntil('\n');
  boardId.trim();
  String values = calibrationFile.readStringUntil('\n');
  calibrationFile.close();
  String expectedId = String(BOARD.id) + " " + TOUCH_CALIBRATION_VERSION;
  if (boardId != expectedId) return false;

  int usesRawX = 0;
  int parsed = sscanf(
    values.c_str(),
    "%d %ld %ld %ld %ld",
    &usesRawX,
    &touchCalibration.xAtLeft,
    &touchCalibration.xAtRight,
    &touchCalibration.yAtTop,
    &touchCalibration.yAtBottom
  );
  touchCalibration.screenXUsesRawX = usesRawX != 0;
  touchCalibration.valid = parsed == 5 &&
    abs(touchCalibration.xAtRight - touchCalibration.xAtLeft) > 1000 &&
    abs(touchCalibration.yAtBottom - touchCalibration.yAtTop) > 1000;
  return touchCalibration.valid;
}

void saveTouchCalibration() {
  File calibrationFile = SPIFFS.open(TOUCH_CALIBRATION_FILE, FILE_WRITE);
  if (!calibrationFile) return;
  calibrationFile.println(String(BOARD.id) + " " + TOUCH_CALIBRATION_VERSION);
  calibrationFile.printf(
    "%d %ld %ld %ld %ld\n",
    touchCalibration.screenXUsesRawX,
    touchCalibration.xAtLeft,
    touchCalibration.xAtRight,
    touchCalibration.yAtTop,
    touchCalibration.yAtBottom
  );
  calibrationFile.close();
}

void drawCalibrationTarget(int16_t x, int16_t y, int number) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  String label = "Touch calibration " + String(number) + "/4";
  tft.setTextSize(uiFittedTextSize(label, 2, tft.width() - 16));
  tft.setCursor(8, tft.height() / 2 - 8);
  tft.print(label);
  tft.drawCircle(x, y, 12, TFT_WHITE);
  tft.drawFastHLine(x - 18, y, 37, TFT_WHITE);
  tft.drawFastVLine(x, y - 18, 37, TFT_WHITE);
}

void captureCalibrationPoint(int16_t x, int16_t y, int number, uint16_t &rawX, uint16_t &rawY) {
  drawCalibrationTarget(x, y, number);
  // Show the target immediately, even if the screen was touched during boot.
  while (rawTouchIsDown()) delay(10);
  while (!rawTouchIsDown()) delay(10);
  delay(35);
  while (!readRawTouchPoint(rawX, rawY)) delay(5);
  while (rawTouchIsDown()) delay(10);
  delay(30);
}

void calibrateTouch() {
  const int16_t margin = 24;
  const int16_t targetX[4] = {margin, int16_t(tft.width() - margin), int16_t(tft.width() - margin), margin};
  const int16_t targetY[4] = {margin, margin, int16_t(tft.height() - margin), int16_t(tft.height() - margin)};
  uint16_t rawX[4];
  uint16_t rawY[4];

  do {
    for (int point = 0; point < 4; point++) {
      captureCalibrationPoint(targetX[point], targetY[point], point + 1, rawX[point], rawY[point]);
    }

    int32_t horizontalRawX = abs(int32_t(rawX[1] + rawX[2]) - int32_t(rawX[0] + rawX[3]));
    int32_t horizontalRawY = abs(int32_t(rawY[1] + rawY[2]) - int32_t(rawY[0] + rawY[3]));
    touchCalibration.screenXUsesRawX = horizontalRawX >= horizontalRawY;

    const uint16_t *screenRawX = touchCalibration.screenXUsesRawX ? rawX : rawY;
    const uint16_t *screenRawY = touchCalibration.screenXUsesRawX ? rawY : rawX;
    int32_t rawAtLeftTarget = (screenRawX[0] + screenRawX[3]) / 2;
    int32_t rawAtRightTarget = (screenRawX[1] + screenRawX[2]) / 2;
    int32_t rawAtTopTarget = (screenRawY[0] + screenRawY[1]) / 2;
    int32_t rawAtBottomTarget = (screenRawY[2] + screenRawY[3]) / 2;
    touchCalibration.xAtLeft = bowserExtrapolateRawAxis(
      rawAtLeftTarget,
      rawAtRightTarget,
      margin,
      tft.width() - margin,
      0
    );
    touchCalibration.xAtRight = bowserExtrapolateRawAxis(
      rawAtLeftTarget,
      rawAtRightTarget,
      margin,
      tft.width() - margin,
      tft.width() - 1
    );
    touchCalibration.yAtTop = bowserExtrapolateRawAxis(
      rawAtTopTarget,
      rawAtBottomTarget,
      margin,
      tft.height() - margin,
      0
    );
    touchCalibration.yAtBottom = bowserExtrapolateRawAxis(
      rawAtTopTarget,
      rawAtBottomTarget,
      margin,
      tft.height() - margin,
      tft.height() - 1
    );
    touchCalibration.valid =
      abs(touchCalibration.xAtRight - touchCalibration.xAtLeft) > 1000 &&
      abs(touchCalibration.yAtBottom - touchCalibration.yAtTop) > 1000;
  } while (!touchCalibration.valid);

  saveTouchCalibration();
  tft.fillScreen(TFT_BLACK);
}

char pollTouchKey() {
  if (!BOARD.hasTouchscreen) return 0;

  char sampledKey = 0;
  uint16_t x = 0;
  uint16_t y = 0;
  if (rawTouchIsDown()) {
    sampledKey = readTouchPoint(x, y) ? keyAtTouchPoint(x, y) : 0;
    if (sampledKey == 0) sampledKey = BOWSER_INVALID_HELD_KEY;
  }
  return touchKeyDebouncer.update(sampledKey, millis(), INPUT_DEBOUNCE_MS);
}

char scanMatrixKeypad() {
  if (!BOARD.hasMatrixKeypad) return 0;
  for (int column = 0; column < 3; column++) {
    int columnPin = BOARD.keypadColumnPins[column];
    pinMode(columnPin, OUTPUT);
    digitalWrite(columnPin, LOW);
    delayMicroseconds(5);

    for (int row = 0; row < 4; row++) {
      if (digitalRead(BOARD.keypadRowPins[row]) == LOW) {
        pinMode(columnPin, INPUT);
        return KEYPAD_KEYS[row][column];
      }
    }
    pinMode(columnPin, INPUT);
  }
  return 0;
}

char pollMatrixKeypad() {
  return matrixKeyDebouncer.update(
    scanMatrixKeypad(),
    millis(),
    INPUT_DEBOUNCE_MS
  );
}

char pollButton(ButtonTracker &button, char shortPressKey, bool longPressCancels) {
  if (button.pin < 0) return 0;
  uint32_t now = millis();
  bool sampledDown = digitalRead(button.pin) == LOW;
  if (sampledDown != button.observedDown) {
    button.observedDown = sampledDown;
    button.changedAt = now;
  }

  if (
    button.observedDown == button.stableDown ||
    uint32_t(now - button.changedAt) < INPUT_DEBOUNCE_MS
  ) return 0;

  button.stableDown = button.observedDown;
  if (button.stableDown) {
    button.pressedAt = now;
  } else {
    unsigned long duration = now - button.pressedAt;
    if (longPressCancels && duration >= LONG_PRESS_CANCEL_MS) return '*';
    return shortPressKey;
  }
  return 0;
}

char pollPhysicalButtons() {
  char key = pollButton(primaryButton, '#', BOARD.singleButtonLongPressCancels);
  if (key != 0) return key;
  return pollButton(secondaryButton, '*', false);
}

bool rawButtonIsDown(const ButtonTracker &button) {
  return button.pin >= 0 && digitalRead(button.pin) == LOW;
}

bool anyRawInputIsDown() {
  return
    rawTouchIsDown() ||
    scanMatrixKeypad() != 0 ||
    rawButtonIsDown(primaryButton) ||
    rawButtonIsDown(secondaryButton);
}

void resetInputDebouncers(uint32_t now) {
  touchKeyDebouncer.reset(now);
  matrixKeyDebouncer.reset(now);
  primaryButton.observedDown = false;
  primaryButton.stableDown = false;
  primaryButton.changedAt = now;
  primaryButton.pressedAt = 0;
  secondaryButton.observedDown = false;
  secondaryButton.stableDown = false;
  secondaryButton.changedAt = now;
  secondaryButton.pressedAt = 0;
}
}

void setupInputHardware() {
  if (inputHardwareInitialized) return;
  inputHardwareInitialized = true;

  if (BOARD.hasMatrixKeypad) {
    for (int row = 0; row < 4; row++) pinMode(BOARD.keypadRowPins[row], INPUT_PULLUP);
    for (int column = 0; column < 3; column++) pinMode(BOARD.keypadColumnPins[column], INPUT);
  }

  primaryButton = {BOARD.primaryButtonPin, false, false, 0, 0};
  secondaryButton = {BOARD.secondaryButtonPin, false, false, 0, 0};
  if (primaryButton.pin >= 0) pinMode(primaryButton.pin, INPUT_PULLUP);
  if (secondaryButton.pin >= 0) pinMode(secondaryButton.pin, INPUT_PULLUP);

  if (BOARD.hasTouchscreen) {
    if (BOARD.gt911.enabled) {
      if (!gt911.begin(BOARD.gt911)) {
        showMessage("GT911 not found", "Check touch hardware");
        while (true) delay(1000);
      }
    } else if (BOARD.xpt2046.enabled) {
      if (!BOARD.xpt2046.sharesDisplaySpi) {
        pinMode(BOARD.xpt2046.clockPin, OUTPUT);
        pinMode(BOARD.xpt2046.mosiPin, OUTPUT);
        pinMode(BOARD.xpt2046.misoPin, INPUT);
        digitalWrite(BOARD.xpt2046.clockPin, LOW);
      }
      pinMode(BOARD.xpt2046.chipSelectPin, OUTPUT);
      pinMode(BOARD.xpt2046.interruptPin, INPUT_PULLUP);
      digitalWrite(BOARD.xpt2046.chipSelectPin, HIGH);
      if (!loadTouchCalibration()) calibrateTouch();
    } else {
      showMessage("Touch profile invalid", BOARD.id);
      while (true) delay(1000);
    }
  }

  resetInputDebouncers(millis());
}

// Sensitive prompts must never accept a control that was already held when
// the prompt appeared. Require a continuously released interval, then reset
// all edge detectors so only a subsequent fresh press can produce an event.
void armInputForPrompt() {
  setupInputHardware();
  StableReleaseGate releaseGate;
  releaseGate.reset(millis());
  while (true) {
    uint32_t now = millis();
    if (releaseGate.update(anyRawInputIsDown(), now, INPUT_DEBOUNCE_MS)) {
      resetInputDebouncers(millis());
      return;
    }
    delay(5);
  }
}

// Compatibility name for the existing dice/seed flows. It initializes the
// input hardware selected by the board profile, not necessarily a keypad.
void setupKeypad() {
  setupInputHardware();
}

char scanKeypad() {
  return scanMatrixKeypad();
}

char pollInputKey() {
  setupInputHardware();
  char key = pollTouchKey();
  if (key != 0) return key;
  key = pollMatrixKeypad();
  if (key != 0) return key;
  return pollPhysicalButtons();
}

char waitForKeypadKey() {
  while (true) {
    char key = pollInputKey();
    if (key != 0) return key;
    delay(10);
  }
}

bool awaitPhysicalReviewApproval() {
  armInputForPrompt();
  while (true) {
    char key = waitForKeypadKey();
    if (key == '#') return true;
    if (key == '*') return false;
  }
}
