//========================================================================//
//================================COMMANDS================================//
//========================================================================//

CommandResponse cmdRes = {"Welcome", "Row, row, row your boat"};


void listenForCommands() {
  if (cmdRes.message != "" || cmdRes.subMessage != "")
    showMessage(cmdRes.message, cmdRes.subMessage);

  String data = awaitSerialData();

  Command c = extractCommand(data);
  if (isEncryptedCommand(c.cmd)) {
    c = decryptAndExtractCommand(data);
  }
  // flush stale data from buffer
  logSerial("received command: " + c.cmd);
  cmdRes = executeCommand(c);

  delay(DELAY_MS);
}

bool isEncryptedCommand(String cmd) {
  return cmd != COMMAND_DH_EXCHANGE &&
         cmd != COMMAND_CHECK_PAIRING &&
         cmd != COMMAND_PING;
}

CommandResponse executeCommand(Command c) {
  if (c.cmd == COMMAND_PING)
    return executePing(c.data);

  if (c.cmd == COMMAND_CHECK_PAIRING)
    return executeCheckSecureConnection(c.data);

  if (c.cmd == COMMAND_DH_EXCHANGE)
    return executeDhExchange(c.data);

  if (c.cmd == COMMAND_HELP)
    return executeHelp(c.data);

  if (c.cmd == COMMAND_WIPE)
    return executeWhipeHww(c.data);

  if (c.cmd == COMMAND_PASSWORD)
    return executePasswordCheck(c.data);

  if (c.cmd == COMMAND_PASSWORD_CLEAR)
    return executePasswordClear(c.data);

  if (c.cmd == COMMAND_SEED)
    return executeShowSeed(c.data);

  if (c.cmd == COMMAND_SEND_PSBT)
    return executeSignPsbt(c.data);

  if (c.cmd == COMMAND_RESTORE)
    return executeRestore(c.data);

  if (c.cmd == COMMAND_XPUB)
    return executeXpub(c.data);

  return executeUnknown(c.data);

}

HwwInitData initHww(String password, String mnemonic) {
  if (isAlphaNumeric(password) == false)
    return {"", "", false};

  deleteFile(SPIFFS, global.mnemonicFileName.c_str());
  deleteFile(SPIFFS, global.passwordFileName.c_str());
  if (mnemonic == "") {
    mnemonic = generateMnemonic(24); // todo: allow 12 also
  }

  String passwordHash  = hashPassword(password);
  writeFile(SPIFFS, global.passwordFileName.c_str(), passwordHash);

  int byteSize =  passwordHash.length() / 2;
  byte encryptionKey[byteSize];
  fromHex(passwordHash, encryptionKey, byteSize);

  writeFile(SPIFFS, global.mnemonicFileName.c_str(), encryptDataWithIv(mnemonic, data));

  return {passwordHash, mnemonic, true};
}

void serialSendCommand(String command, String commandData) {
  serialPrintlnSecure(command + " " + commandData);
}

void serialPrintlnSecure(String msg) {
  logSerial("### serialPrintlnSecure.dhe_shared_secret: " + toHex(global.dhe_shared_secret, sizeof(global.dhe_shared_secret)));

  String encryptedHex = encryptDataWithIv(global.dhe_shared_secret, msg);
  Serial.println(encryptedHex);
}

Command decryptAndExtractCommand(String ecryptedData) {
  String data = decryptDataWithIv(global.dhe_shared_secret, ecryptedData);
  return extractCommand(data);
}
