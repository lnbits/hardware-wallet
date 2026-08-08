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

bool hardwareRngPassesHealthCheck() {
  // This checks conditioned ESP RNG output for gross failures. It does not
  // measure the entropy of the underlying physical noise source.
  const size_t sampleCount = 64;
  const size_t totalBits = sampleCount * 32;
  uint32_t samples[sampleCount];
  esp_fill_random(samples, sizeof(samples));

  uint32_t onesSeen = 0;
  uint32_t zerosSeen = 0;
  size_t oneBitCount = 0;
  size_t repeatedWords = 1;
  size_t longestRepeat = 1;

  for (size_t i = 0; i < sampleCount; i++) {
    uint32_t sample = samples[i];
    onesSeen |= sample;
    zerosSeen |= ~sample;

    while (sample != 0) {
      oneBitCount++;
      sample &= sample - 1;
    }

    if (i > 0 && samples[i] == samples[i - 1]) {
      repeatedWords++;
      if (repeatedWords > longestRepeat) longestRepeat = repeatedWords;
    } else {
      repeatedWords = 1;
    }
  }

  bool allBitsChanged = onesSeen == UINT32_MAX && zerosSeen == UINT32_MAX;
  bool noStuckOutput = longestRepeat < 3;
  bool plausibleBitBalance =
    oneBitCount >= (totalBits * 2) / 5 &&
    oneBitCount <= (totalBits * 3) / 5;

  clearSensitiveBytes((uint8_t *)samples, sizeof(samples));
  return allBitsChanged && noStuckOutput && plausibleBitBalance;
}

String generateExtraEtropy() {
  bootloader_random_enable();

  if (hardwareRngPassesHealthCheck() == false) {
    bootloader_random_disable();
    logInfo("Hardware RNG health check failed");
    return "";
  }

  String uBitcoinEntropy = generateMnemonic(24);
  if (uBitcoinEntropy == "") {
    bootloader_random_disable();
    logInfo("Hardware RNG mnemonic generation failed");
    return "";
  }

  byte espEntropy[32];
  esp_fill_random(espEntropy, 32);
  String espHexEntropy = toHex(espEntropy, 32);
  clearSensitiveBytes(espEntropy, sizeof(espEntropy));

  String clientEntropy = toHex(global.dhe_shared_secret, 32);

  bootloader_random_disable();

  return uBitcoinEntropy + espHexEntropy + clientEntropy + global.passwordVerifier;
}

String generateStrongerMnemonic(int wordCount) {
  String extraEtropy = generateExtraEtropy();
  if (extraEtropy == "") return "";

  const char *mnemonic = generateMnemonic(wordCount, extraEtropy);
  if (mnemonic == NULL) return "";
  return String(mnemonic);
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
