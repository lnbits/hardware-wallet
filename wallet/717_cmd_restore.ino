/**
   @brief Restore a wallet from a BIP39 word list.
   A new password must be provided.
   @param commandData: String. Space separated values. Use minus (`-`) to skip the value.
    Value significance by position:
     0 - password: String. Minimum 8 characters, spaces not allowed.
     1 - mnemonic: String. BIP39 word list.
   @return CommandResponse
      - restore status to the UI.
      - `/restore {status}` to the client. Status is `1` if restore is successful, `0` otherwhise.
*/
CommandResponse executeRestore(String commandData) {
  int spacePos = commandData.indexOf(" ");
  String password = commandData.substring(0, spacePos);
  String mnemonic = commandData.substring(spacePos + 1, commandData.length() );

  if (isEmptyParam(mnemonic)) {
    return { "Enter seed words",  "Separated by spaces"};
  }
  if (isEmptyParam(password)) {
    return { "Cannot restore",  "Password missing"};
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

  HwwInitData data = initHww(password, mnemonic, global.passphrase, global.persistSecrets);
  delay(DELAY_MS);
  global.authenticated = data.success;
  serialSendCommand(COMMAND_RESTORE, String(global.authenticated));

  if (global.authenticated == true) {
    global.passwordHash = data.passwordHash;
    global.mnemonic = data.mnemonic;
    return {"Restore successful",  "/seed` to view word list"};
  }
  return { "Error, try again", "8 numbers/letters"};
}
