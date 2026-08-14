#pragma once

#include <stdint.h>

// A non-zero value that can represent a held input which is not a valid key,
// for example a touch that moved outside its original button.
constexpr char BOWSER_INVALID_HELD_KEY = '\x01';

inline int32_t bowserExtrapolateRawAxis(
  int32_t rawNear,
  int32_t rawFar,
  int16_t screenNear,
  int16_t screenFar,
  int16_t screenTarget
) {
  int32_t screenSpan = screenFar - screenNear;
  if (screenSpan == 0) return rawNear;
  return rawNear +
    (rawFar - rawNear) * (screenTarget - screenNear) / screenSpan;
}

class ReleasedKeyDebouncer {
 public:
  ReleasedKeyDebouncer()
    : observedKey_(0), stableKey_(0), pressedKey_(0), changedAt_(0) {}

  void reset(uint32_t now) {
    observedKey_ = 0;
    stableKey_ = 0;
    pressedKey_ = 0;
    changedAt_ = now;
  }

  char update(char sampledKey, uint32_t now, uint32_t debounceMs) {
    if (sampledKey != observedKey_) {
      observedKey_ = sampledKey;
      changedAt_ = now;
    }

    if (
      observedKey_ == stableKey_ ||
      uint32_t(now - changedAt_) < debounceMs
    ) return 0;

    char previousStableKey = stableKey_;
    stableKey_ = observedKey_;

    if (previousStableKey == 0 && stableKey_ != 0) {
      pressedKey_ = stableKey_;
      return 0;
    }

    if (previousStableKey != 0 && stableKey_ == 0) {
      char releasedKey = pressedKey_ == previousStableKey ? pressedKey_ : 0;
      pressedKey_ = 0;
      return releasedKey == BOWSER_INVALID_HELD_KEY ? 0 : releasedKey;
    }

    // A finger or multi-key press moved from one key to another without a
    // stable release. Invalidate the entire gesture rather than guessing.
    pressedKey_ = 0;
    return 0;
  }

 private:
  char observedKey_;
  char stableKey_;
  char pressedKey_;
  uint32_t changedAt_;
};

class StableReleaseGate {
 public:
  StableReleaseGate() : releasedAt_(0) {}

  void reset(uint32_t now) {
    releasedAt_ = now;
  }

  bool update(bool anyInputDown, uint32_t now, uint32_t settleMs) {
    if (anyInputDown) {
      releasedAt_ = now;
      return false;
    }
    return uint32_t(now - releasedAt_) >= settleMs;
  }

 private:
  uint32_t releasedAt_;
};
