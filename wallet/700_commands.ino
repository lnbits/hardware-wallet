//========================================================================//
//================================COMMANDS================================//
//========================================================================//



CommandResponse cmdRes = {"Welcome", "Row, row, row your boat"};


void listenForCommands() {
  // todo: called too many times
  if (loadFiles() == false) {
    cmdRes = { "Failed opening files",  "Reset or 'help'"};
  }

  if (cmdRes.message != "" || cmdRes.subMessage != "")
    showMessage(cmdRes.message, cmdRes.subMessage);

  String data = awaitSerialData();

  Command c = extractCommand(data);
  if (c.cmd != COMMAND_DH_EXCHANGE) {
    // if (!g.dhe_shared_secret)
    //   return { "No secure connection",  "Please reconnect"};
    data = decryptMessageWithIv(g.dhe_shared_secret, data);
    c = extractCommand(data);
  }
  // flush stale data from buffer
  logSerial("received command: " + c.cmd);
  cmdRes = executeCommand(c);

  delay(DELAY_MS);
}


CommandResponse executeCommand(Command c) {
  if (c.cmd == COMMAND_DH_EXCHANGE)
    return executeDhExchange(c.data);

  if (c.cmd == COMMAND_HELP)
    return executeHelp(c.data);

  if (c.cmd == COMMAND_WIPE)
    return executeinitHww(c.data);

  if (c.cmd == COMMAND_PASSWORD)
    return executePasswordCheck(c.data);

  if (c.cmd == COMMAND_PASSWORD_CLEAR)
    return executePasswordClear(c.data);

  if (c.cmd == COMMAND_SEED)
    return executeShowSeed(c.data);

  if (c.cmd == COMMAND_SEND_PSBT)
    return executeSignPsbt(c.data);

  if (c.cmd == COMMAND_RESTORE)
    return executeRestore(c.data, "");

  if (c.cmd == COMMAND_XPUB)
    return executeXpub(c.data);

  return executeUnknown(c.data);

}


bool loadFiles() {
  FileData mnFile = readFile(SPIFFS, "/mn.txt");
  g.encrytptedMnemonic = mnFile.data;

  FileData pwdFile = readFile(SPIFFS, "/hash.txt");
  g.passwordHash = pwdFile.data;

  return mnFile.success && pwdFile.success;
}

bool initHww(String password, String mnemonic) {
  if (isAlphaNumeric(password) == false)
    return false;

  deleteFile(SPIFFS, "/mn.txt");
  deleteFile(SPIFFS, "/hash.txt");
  if (mnemonic == "") {
    mnemonic = createMnemonic(24); // todo: allow 12 also
  }
  g.passwordHash = hashPassword(password);
  writeFile(SPIFFS, "/mn.txt", mnemonic);
  writeFile(SPIFFS, "/hash.txt", g.passwordHash);

  delay(DELAY_MS);
  return true;
}

void serialSendCommand(String command, String commandData) {
  serialPrintlnSecure(command + " " + commandData);
}

void serialPrintlnSecure(String msg) {
  String data = String(msg.length()) + " " + msg;
  // pad data
  while (data.length() % 16 != 0) data += " ";

  // create random initialization vector
  int ivSize = 16;
  uint8_t iv[ivSize];
  String tempMnemonic = createMnemonic(24);
  mnemonicToEntropy(tempMnemonic, iv, ivSize);
  String ivHex = toHex(iv, ivSize);

  String messageHex = encryptData(g.dhe_shared_secret, ivHex, data);

  Serial.println(messageHex + ivHex);
}

