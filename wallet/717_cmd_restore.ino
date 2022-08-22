CommandResponse executeRestore(String commandData) {
  int spacePos = commandData.indexOf(" ");
  String password = commandData.substring(0, spacePos);
  String mnemonicWithPassphrase = commandData.substring(spacePos + 1, commandData.length() );
  SeedData seedData = parseSeedData(mnemonicWithPassphrase);

  if (seedData.mnemonic == "") {
    return { "Enter seed words",  "Separated by spaces"};
  }
  if (password == "") {
    return { "Cannot restore",  "New data missing"};
  }



  int size = getMnemonicBytes(seedData.mnemonic);
  if (size == 0) {
    serialSendCommand(COMMAND_RESTORE, "0");
    return {"Wrong word count!", "Must be 12, 15, 18, 21 or 24"};
  }

  if (!checkMnemonic(seedData.mnemonic)) {
    serialSendCommand(COMMAND_RESTORE, "0");
    return {"Wrong mnemonic!", "Incorrect checksum"};
  }

  HwwInitData data = initHww(password, seedData.mnemonic, seedData.passphrase);
  delay(DELAY_MS);
  global.authenticated = data.success;
  serialSendCommand(COMMAND_RESTORE, String(global.authenticated));

  if (global.authenticated == true) {
    global.passwordHash = data.passwordHash;
    global.mnemonic = data.mnemonic;
    global.passphrase = seedData.passphrase;
    return {"Restore successfull",  "/seed` to view word list"};
  }
  return { "Error, try again", "8 numbers/letters"};
}
