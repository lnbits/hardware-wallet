#include <cassert>
#include <cstddef>
#include <cstdint>

#include "../wallet/rng_health.h"

namespace {

void fillDeterministicHealthySample(uint32_t *samples, size_t count) {
  uint32_t state = 0x6d2b79f5U;
  for (size_t i = 0; i < count; i++) {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    samples[i] = state;
  }
}

void healthyConditionedOutputPasses() {
  uint32_t samples[BowserRngHealth::kSampleWordCount];
  fillDeterministicHealthySample(samples, BowserRngHealth::kSampleWordCount);
  assert(BowserRngHealth::conditionedOutputPasses(
    samples,
    BowserRngHealth::kSampleWordCount
  ));
}

void insufficientSampleFailsClosed() {
  uint32_t samples[BowserRngHealth::kSampleWordCount];
  fillDeterministicHealthySample(samples, BowserRngHealth::kSampleWordCount);
  assert(!BowserRngHealth::conditionedOutputPasses(
    samples,
    BowserRngHealth::kSampleWordCount - 1
  ));
  assert(!BowserRngHealth::conditionedOutputPasses(NULL, 0));
}

void stuckOutputFails() {
  uint32_t samples[BowserRngHealth::kSampleWordCount] = {0};
  assert(!BowserRngHealth::conditionedOutputPasses(
    samples,
    BowserRngHealth::kSampleWordCount
  ));
}

void stuckBitPositionFails() {
  uint32_t samples[BowserRngHealth::kSampleWordCount];
  fillDeterministicHealthySample(samples, BowserRngHealth::kSampleWordCount);
  for (size_t i = 0; i < BowserRngHealth::kSampleWordCount; i++) {
    samples[i] &= ~1U;
  }
  assert(!BowserRngHealth::conditionedOutputPasses(
    samples,
    BowserRngHealth::kSampleWordCount
  ));
}

void consecutiveDuplicateWordFails() {
  uint32_t samples[BowserRngHealth::kSampleWordCount];
  fillDeterministicHealthySample(samples, BowserRngHealth::kSampleWordCount);
  samples[65] = samples[64];
  assert(!BowserRngHealth::conditionedOutputPasses(
    samples,
    BowserRngHealth::kSampleWordCount
  ));
}

void shortPeriodFails() {
  uint32_t samples[BowserRngHealth::kSampleWordCount];
  for (size_t i = 0; i < BowserRngHealth::kSampleWordCount; i++) {
    samples[i] = i % 2 == 0 ? 0x96a53c78U : 0x4bd2e10fU;
  }
  assert(!BowserRngHealth::conditionedOutputPasses(
    samples,
    BowserRngHealth::kSampleWordCount
  ));
}

void excessiveBitRunFails() {
  uint32_t samples[BowserRngHealth::kSampleWordCount];
  fillDeterministicHealthySample(samples, BowserRngHealth::kSampleWordCount);
  samples[0] = 0;
  assert(!BowserRngHealth::conditionedOutputPasses(
    samples,
    BowserRngHealth::kSampleWordCount
  ));
}

void biasedAdaptiveWindowFails() {
  uint32_t samples[BowserRngHealth::kSampleWordCount];
  fillDeterministicHealthySample(samples, BowserRngHealth::kSampleWordCount);
  for (size_t i = 0; i < 32; i++) {
    // Alternating words avoid a long identical-bit run while making the first
    // 1024-bit adaptive-proportion window overwhelmingly one-heavy.
    samples[i] = i % 2 == 0 ? 0xeeeeeeeeU : 0xddddddddU;
  }
  assert(!BowserRngHealth::conditionedOutputPasses(
    samples,
    BowserRngHealth::kSampleWordCount
  ));
}

}  // namespace

int main() {
  healthyConditionedOutputPasses();
  insufficientSampleFailsClosed();
  stuckOutputFails();
  stuckBitPositionFails();
  consecutiveDuplicateWordFails();
  shortPeriodFails();
  excessiveBitRunFails();
  biasedAdaptiveWindowFails();
  return 0;
}
