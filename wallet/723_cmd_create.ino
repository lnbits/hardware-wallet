/**
   @brief Create a 24-word BIP39 wallet from physical six-sided dice rolls.

   This command is intentionally restricted to SD-card command mode. The user
   enters 100 rolls on the attached 4x3 keypad. The SHA-256 digest of the 100
   ASCII digits is used directly as the 256-bit BIP39 entropy. The rolls and
   mnemonic are never written to the SD card.

   @param password: String. Minimum 8 characters, spaces are not allowed.
   @return CommandResponse
      - creation and seed-review status to the UI.
      - `/create {status}` to the SD-card output file. Status is `1` when the
        wallet is successfully created and `0` otherwise.
*/
CommandResponse executeCreate(String password) {
  if (global.hasCommandsFile == false) {
    return {"SD card only", "Use commands.in.txt"};
  }

  password.trim();
  if (
    isEmptyParam(password) ||
    password.indexOf(' ') != -1 ||
    isValidPassword(password) == false
  ) {
    sendCommandOutput(COMMAND_CREATE, "0");
    return {"Cannot create", "Password: 8+ chars"};
  }

  char rolls[DICE_ROLL_COUNT];
  int rollCount = collectDiceRolls(rolls, DICE_ROLL_COUNT);
  if (rollCount != DICE_ROLL_COUNT) {
    clearSensitiveBytes((uint8_t *)rolls, sizeof(rolls));
    sendCommandOutput(COMMAND_CREATE, "0");
    return {"Cannot create", "Dice entry failed"};
  }

  uint8_t entropy[32];
  sha256((uint8_t *)rolls, sizeof(rolls), entropy);
  clearSensitiveBytes((uint8_t *)rolls, sizeof(rolls));

  const char *mnemonicChars = mnemonicFromEntropy(entropy, sizeof(entropy));
  String mnemonic = mnemonicChars == NULL ? "" : String(mnemonicChars);
  clearSensitiveBytes(entropy, sizeof(entropy));
  if (mnemonic == "" || checkMnemonic(mnemonic) == false) {
    sendCommandOutput(COMMAND_CREATE, "0");
    return {"Cannot create", "Mnemonic error"};
  }

  showMessage("Creating wallet", "Please wait...");
  HwwInitData data = initHww(
    password,
    mnemonic,
    "",
    true
  );
  mnemonic = "";
  delay(DELAY_MS);

  global.authenticated = data.success;
  if (global.authenticated == false) {
    sendCommandOutput(COMMAND_CREATE, "0");
    return {"Cannot create", "Wallet init failed"};
  }

  global.passwordHash = data.passwordHash;
  global.mnemonic = data.mnemonic;
  global.passphrase = "";

  reviewDiceMnemonic(global.mnemonic);
  sendCommandOutput(COMMAND_CREATE, "1");
  return {"Wallet created", "24 words reviewed"};
}

int collectDiceRolls(char *rolls, int requiredRolls) {
  setupKeypad();
  int rollCount = 0;
  showDiceRollProgress(rollCount, 0);

  while (true) {
    while (rollCount < requiredRolls) {
      char key = waitForKeypadKey();
      if (key >= '1' && key <= '6') {
        rolls[rollCount] = key;
        rollCount++;
        showDiceRollProgress(rollCount, key);
      } else if (key == '*' && rollCount > 0) {
        rollCount--;
        rolls[rollCount] = 0;
        showDiceRollProgress(rollCount, 0);
      }
    }

    showDiceRollProgress(rollCount, 0);
    char key = waitForKeypadKey();
    if (key == '#') {
      return rollCount;
    }
    if (key == '*') {
      rollCount--;
      rolls[rollCount] = 0;
      showDiceRollProgress(rollCount, 0);
    }
  }
}

void reviewDiceMnemonic(const String &mnemonic) {
  int position = 1;
  while (true) {
    showDiceMnemonicWord(position, getWordAtPosition(mnemonic, position - 1));
    char key = waitForKeypadKey();

    if (key == '*' && position > 1) {
      position--;
    } else if (key == '#' && position < 24) {
      position++;
    } else if (key == '#' && position == 24) {
      return;
    }
  }
}

void clearSensitiveBytes(uint8_t *data, size_t length) {
  volatile uint8_t *cursor = data;
  while (length-- > 0) {
    *cursor++ = 0;
  }
}
