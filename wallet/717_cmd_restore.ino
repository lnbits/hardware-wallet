CommandResponse executeRestore(String mnemonic, String password) {
  logSerial("Execute Restore");
  if (mnemonic == "") {
    return { "Enter seed words",  "Separated by spaces"};
  }

  int size = getMnemonicBytes(mnemonic);
  if (size == 0) {
    serialSendCommand(COMMAND_RESTORE, "0");
    return {"Wrong word count!", "Must be 12, 15, 18, 21 or 24"};
  }

  if (!checkMnemonic(mnemonic)) {
    serialSendCommand(COMMAND_RESTORE, "0");
    return {"Wrong mnemonic!", "Incorrect checksum"};
  }

  if (password == "") {
    showMessage("Enter new password!", "8 numbers/letters");
    String data = awaitSerialData();
    Command c = decryptAndExtractCommand(data);

    if (c.cmd != COMMAND_PASSWORD) {
      return executeUnknown("");
    }

    password = c.data;
  }

  HwwInitData data = initHww(password, mnemonic);
  delay(DELAY_MS);
  global.authenticated = data.success;
  serialSendCommand(COMMAND_RESTORE, String(global.authenticated));

  if (global.authenticated == true) {
    global.passwordHash = data.passwordHash;
    global.encrytptedMnemonic = data.mnemonic;
    return {"Restore successfull",  "/seed` to view word list"};
  }
  return { "Error, try again", "8 numbers/letters"};
}
