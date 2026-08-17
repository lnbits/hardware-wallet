//========================================================================//
//================================HELPERS=================================//
//========================================================================//

int getMnemonicBytes(String menmonicSentence) {
  int wc = wordCount(menmonicSentence);
  switch (wc)
  {
    case 12:
      return 16;
    case 15:
      return 20;
    case 18:
      return 24;
    case 21:
      return 28;
    case 24:
      return 32;
    default:
      return 0;
  }
}

int wordCount(String s) {
  int count = 1;
  for (int i = 0; i < s.length(); i++)
    if (s[i] == ' ') count++;
  return count;
}


bool isValidPassword(String instr) {
  if (instr.length() < 8) {
    return false;
  }
  return true;
}

String int64ToString(uint64_t value) {
  String s = "";
  while (value != 0) {
    s = String((int)(value % 10)) + s;
    value = (uint64_t) value / 10;
  }
  return s;
}

String getWordAtPosition(String str, int position) {
  return getTokenAtPosition(str, " ", position);
}

String getLineAtPosition(String str, int position) {
  return getTokenAtPosition(str, "\n", position);
}

String getTokenAtPosition(String str, String separator, int position) {
  String s = str.substring(0);
  int separatorPos = 0;
  int i = 0;
  while (separatorPos != -1) {
    separatorPos = s.indexOf(separator);
    if (i == position) {
      if (separatorPos == -1) return s;
      return s.substring(0, separatorPos);
    }
    s = s.substring(separatorPos + 1);
    i++;
  }

  return "";
}

bool deriveHealthyHardwareEntropy(uint8_t *output, size_t outputLength) {
  if (output == NULL || outputLength == 0 || outputLength > 32) return false;

  uint32_t samples[BowserRngHealth::kSampleWordCount] = {0};
  uint8_t digest[32] = {0};
  memset(output, 0, outputLength);

  // Espressif documents this as enabling a physical-noise source when RF is
  // not active. Keep enable/disable paired inside this function so callers
  // cannot accidentally sample the PRNG-only state.
  bootloader_random_enable();
  esp_fill_random(samples, sizeof(samples));
  bootloader_random_disable();

  // These checks run on the ESP hardware RNG's conditioned output. They are
  // defense in depth, not a claim of SP 800-90B validation of the inaccessible
  // raw physical-noise samples.
  const bool healthy = BowserRngHealth::conditionedOutputPasses(
    samples,
    BowserRngHealth::kSampleWordCount
  );
  if (!healthy) {
    clearSensitiveBytes((uint8_t *)samples, sizeof(samples));
    logInfo("Hardware RNG health check failed");
    return false;
  }

  // Condition the complete, health-checked sample into at most 256 bits. No
  // password verifier, pairing secret, or other wallet/session state enters
  // this derivation.
  if (wally_sha256((uint8_t *)samples, sizeof(samples), digest, sizeof(digest)) != WALLY_OK) {
    clearSensitiveBytes(digest, sizeof(digest));
    clearSensitiveBytes((uint8_t *)samples, sizeof(samples));
    return false;
  }
  memcpy(output, digest, outputLength);
  clearSensitiveBytes(digest, sizeof(digest));
  clearSensitiveBytes((uint8_t *)samples, sizeof(samples));
  return true;
}

String generateStrongerMnemonic(int wordCount) {
  const size_t entropyLength = getMnemonicBytesForWordCount(wordCount);
  if (entropyLength == 0) return "";
  uint8_t entropy[32] = {0};
  if (!deriveHealthyHardwareEntropy(entropy, entropyLength)) return "";

  String mnemonic = mnemonicFromBytes(entropy, entropyLength);
  clearSensitiveBytes(entropy, sizeof(entropy));
  return mnemonic;
}

bool isNotEmptyParam(String paramValue) {
  return paramValue && paramValue != "" && paramValue != "-";
}

bool isEmptyParam(String paramValue) {
  return !isNotEmptyParam(paramValue);
}

bool isStrictHex(const String &value) {
  if (value.length() == 0 || value.length() % 2 != 0) return false;
  for (size_t i = 0; i < value.length(); i++) {
    char c = value[i];
    if (!(
      (c >= '0' && c <= '9') ||
      (c >= 'a' && c <= 'f') ||
      (c >= 'A' && c <= 'F')
    )) return false;
  }
  return true;
}

Command extractCommand(String s) {
  int spacePos = s.indexOf(" ");
  String command = s.substring(0, spacePos);
  if (spacePos == -1) {
    return {command, ""};
  }
  String commandData = s.substring(spacePos + 1, s.length());
  return {command, commandData};
}
