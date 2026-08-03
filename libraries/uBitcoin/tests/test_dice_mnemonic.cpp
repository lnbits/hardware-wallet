#ifdef UBTC_TEST

#include "minunit.h"
#include "Bitcoin.h"
#include "Hash.h"

#include <cstring>
#include <string>

MU_TEST(test_dice_rolls_produce_24_word_mnemonic) {
  uint8_t rolls[100];
  for (size_t i = 0; i < sizeof(rolls); i++) {
    rolls[i] = '1' + (i % 6);
  }

  uint8_t entropy[32];
  sha256(rolls, sizeof(rolls), entropy);

  std::string mnemonic = mnemonicFromEntropy(entropy, sizeof(entropy));
  mu_assert(checkMnemonic(mnemonic), "dice mnemonic should pass BIP39 checksum");

  size_t wordCount = 1;
  for (size_t i = 0; i < mnemonic.length(); i++) {
    if (mnemonic[i] == ' ') {
      wordCount++;
    }
  }
  mu_assert(wordCount == 24, "256-bit dice entropy should produce 24 words");

  uint8_t recoveredEntropy[32];
  size_t recoveredLength = mnemonicToEntropy(
    mnemonic,
    recoveredEntropy,
    sizeof(recoveredEntropy)
  );
  mu_assert(recoveredLength == sizeof(entropy), "recovered entropy should be 32 bytes");
  mu_assert(
    std::memcmp(entropy, recoveredEntropy, sizeof(entropy)) == 0,
    "mnemonic should encode the SHA-256 dice entropy exactly"
  );
}

MU_TEST_SUITE(test_dice_mnemonic) {
  MU_RUN_TEST(test_dice_rolls_produce_24_word_mnemonic);
}

int main(int argc, char *argv[]) {
  MU_RUN_SUITE(test_dice_mnemonic);
  MU_REPORT();
  return MU_EXIT_CODE;
}

#endif
