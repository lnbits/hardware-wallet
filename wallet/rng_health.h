#pragma once

#include <stddef.h>
#include <stdint.h>

namespace BowserRngHealth {

// Four 1024-bit windows keep the startup test small enough for an ESP32 while
// providing substantially more coverage than the old 2048-bit aggregate test.
static const size_t kSampleWordCount = 128;
static const size_t kAdaptiveWindowBits = 1024;

// SP 800-90B-inspired conservative cutoffs for binary conditioned output.
// A run of 31 equal bits is rejected (alpha approximately 2^-30 for one run).
// In each 1024-bit window, 609 occurrences of either bit are rejected (the
// one-sided binomial tail is below 2^-30 for an ideal binary source).
static const size_t kRepetitionCutoff = 31;
static const size_t kAdaptiveCutoff = 609;

// Detect repeating multi-word patterns that bit-frequency tests can miss.
static const size_t kMaximumCheckedPeriodWords = 16;

inline bool conditionedOutputPasses(
  const uint32_t *samples,
  size_t sampleWordCount
) {
  if (
    samples == NULL ||
    sampleWordCount < kSampleWordCount ||
    sampleWordCount % (kAdaptiveWindowBits / 32) != 0
  ) return false;

  uint32_t onesSeen = 0;
  uint32_t zerosSeen = 0;
  bool previousBit = false;
  bool havePreviousBit = false;
  size_t repetitionCount = 0;
  size_t windowBitCount = 0;
  size_t windowOneCount = 0;

  for (size_t wordIndex = 0; wordIndex < sampleWordCount; wordIndex++) {
    const uint32_t word = samples[wordIndex];
    onesSeen |= word;
    zerosSeen |= ~word;

    if (wordIndex > 0 && word == samples[wordIndex - 1]) return false;

    for (size_t bitIndex = 0; bitIndex < 32; bitIndex++) {
      const bool bit = ((word >> bitIndex) & 1U) != 0;
      if (havePreviousBit && bit == previousBit) {
        repetitionCount++;
      } else {
        previousBit = bit;
        havePreviousBit = true;
        repetitionCount = 1;
      }
      if (repetitionCount >= kRepetitionCutoff) return false;

      if (bit) windowOneCount++;
      windowBitCount++;
      if (windowBitCount == kAdaptiveWindowBits) {
        const size_t windowZeroCount = kAdaptiveWindowBits - windowOneCount;
        if (
          windowOneCount >= kAdaptiveCutoff ||
          windowZeroCount >= kAdaptiveCutoff
        ) return false;
        windowBitCount = 0;
        windowOneCount = 0;
      }
    }
  }

  // Every output bit position must have produced both states.
  if (onesSeen != UINT32_MAX || zerosSeen != UINT32_MAX) return false;

  const size_t maximumPeriod =
    sampleWordCount - 1 < kMaximumCheckedPeriodWords
      ? sampleWordCount - 1
      : kMaximumCheckedPeriodWords;
  for (size_t period = 2; period <= maximumPeriod; period++) {
    for (size_t i = period + 1; i < sampleWordCount; i++) {
      if (
        samples[i] == samples[i - period] &&
        samples[i - 1] == samples[i - period - 1]
      ) return false;
    }
  }

  return windowBitCount == 0;
}

}  // namespace BowserRngHealth
