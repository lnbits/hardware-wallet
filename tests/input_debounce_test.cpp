#include <cassert>
#include <cstdint>

#include "../wallet/input_debounce.h"

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
  return 0;
}
