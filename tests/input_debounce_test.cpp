#include <cassert>
#include <cstdint>

#include "../wallet/input_debounce.h"
#include "../wallet/touch_transform.h"

namespace {
constexpr uint32_t debounceMs = 30;

void pressBounceProducesOneRelease() {
  ReleasedKeyDebouncer input;
  input.reset(0);
  assert(input.update('1', 1, debounceMs) == 0);
  assert(input.update(0, 5, debounceMs) == 0);
  assert(input.update('1', 8, debounceMs) == 0);
  assert(input.update('1', 38, debounceMs) == 0);
  assert(input.update(0, 40, debounceMs) == 0);
  assert(input.update('1', 44, debounceMs) == 0);
  assert(input.update(0, 48, debounceMs) == 0);
  assert(input.update(0, 78, debounceMs) == '1');
  assert(input.update(0, 120, debounceMs) == 0);
}

void shortGlitchProducesNoRelease() {
  ReleasedKeyDebouncer input;
  input.reset(0);
  assert(input.update('2', 1, debounceMs) == 0);
  assert(input.update(0, 10, debounceMs) == 0);
  assert(input.update(0, 50, debounceMs) == 0);
}

void movingBetweenKeysCancelsGesture() {
  ReleasedKeyDebouncer input;
  input.reset(0);
  assert(input.update('3', 1, debounceMs) == 0);
  assert(input.update('3', 31, debounceMs) == 0);
  assert(input.update('4', 32, debounceMs) == 0);
  assert(input.update('4', 62, debounceMs) == 0);
  assert(input.update(0, 63, debounceMs) == 0);
  assert(input.update(0, 93, debounceMs) == 0);
}

void draggingOutsideTouchButtonCancelsGesture() {
  ReleasedKeyDebouncer input;
  input.reset(0);
  assert(input.update('#', 1, debounceMs) == 0);
  assert(input.update('#', 31, debounceMs) == 0);
  assert(input.update(BOWSER_INVALID_HELD_KEY, 32, debounceMs) == 0);
  assert(input.update(BOWSER_INVALID_HELD_KEY, 62, debounceMs) == 0);
  assert(input.update(0, 63, debounceMs) == 0);
  assert(input.update(0, 93, debounceMs) == 0);
}

void timerWraparoundIsSafe() {
  ReleasedKeyDebouncer input;
  input.reset(UINT32_MAX - 20);
  assert(input.update('5', UINT32_MAX - 10, debounceMs) == 0);
  assert(input.update('5', 20, debounceMs) == 0);
  assert(input.update(0, 21, debounceMs) == 0);
  assert(input.update(0, 51, debounceMs) == '5');
}

void heldInputCannotArmPrompt() {
  StableReleaseGate gate;
  gate.reset(0);
  assert(!gate.update(true, 20, debounceMs));
  assert(!gate.update(false, 40, debounceMs));
  assert(!gate.update(true, 55, debounceMs));
  assert(!gate.update(false, 84, debounceMs));
  assert(gate.update(false, 85, debounceMs));
}

void releaseGateWraparoundIsSafe() {
  StableReleaseGate gate;
  gate.reset(UINT32_MAX - 10);
  assert(!gate.update(false, 5, debounceMs));
  assert(gate.update(false, 20, debounceMs));
}

void calibrationTargetsAreExtrapolatedToScreenEdges() {
  assert(bowserExtrapolateRawAxis(500, 3500, 24, 296, 0) == 236);
  assert(bowserExtrapolateRawAxis(500, 3500, 24, 296, 319) == 3753);
  assert(bowserExtrapolateRawAxis(3500, 500, 24, 296, 0) == 3764);
  assert(bowserExtrapolateRawAxis(3500, 500, 24, 296, 319) == 247);
}

void gt911BoardTransformMapsCornersAndCenter() {
  uint16_t x = 0;
  uint16_t y = 0;

  // ESP32-3248S035C: portrait 320x480 controller, landscape 480x320 UI,
  // with swapped axes and inverted display Y.
  assert(bowserMapTouchPoint(
    0, 0, 320, 480, true, false, true, 480, 320, x, y
  ));
  assert(x == 0);
  assert(y == 319);

  assert(bowserMapTouchPoint(
    319, 479, 320, 480, true, false, true, 480, 320, x, y
  ));
  assert(x == 479);
  assert(y == 0);

  assert(bowserMapTouchPoint(
    160, 240, 320, 480, true, false, true, 480, 320, x, y
  ));
  assert(x == 240);
  assert(y == 159);
}

void touchTransformClampsAndRejectsInvalidGeometry() {
  uint16_t x = 0;
  uint16_t y = 0;

  assert(bowserMapTouchPoint(
    500, 600, 320, 480, false, true, false, 320, 480, x, y
  ));
  assert(x == 0);
  assert(y == 479);

  assert(!bowserMapTouchPoint(
    0, 0, 1, 480, false, false, false, 320, 480, x, y
  ));
  assert(!bowserMapTouchPoint(
    0, 0, 320, 480, false, false, false, 1, 480, x, y
  ));
}
}

int main() {
  pressBounceProducesOneRelease();
  shortGlitchProducesNoRelease();
  movingBetweenKeysCancelsGesture();
  draggingOutsideTouchButtonCancelsGesture();
  timerWraparoundIsSafe();
  heldInputCannotArmPrompt();
  releaseGateWraparoundIsSafe();
  calibrationTargetsAreExtrapolatedToScreenEdges();
  gt911BoardTransformMapsCornersAndCenter();
  touchTransformClampsAndRejectsInvalidGeometry();
  return 0;
}
