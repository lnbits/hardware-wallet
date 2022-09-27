//========================================================================//
//================================COMMANDS================================//
//========================================================================//

CommandResponse cmdRes = {"Welcome", "Row, row, row your boat"};


void listenForCommands() {
  if (cmdRes.message != "" || cmdRes.subMessage != "")
    showMessage(cmdRes.message, cmdRes.subMessage);


  // if the command does not handle an event then it bubbles it up
  EventData event = cmdRes.event;
  if (isNotCommandEvent(event.type)) {
    event = awaitEvent();
  }

  if (isNotCommandEvent(event.type)) return;

  String data = event.data;

  Command c = extractCommand(data);
  if (isEncryptedCommand(c.cmd) && isNotInternalCommand(event.type)) {
    c = decryptAndExtractCommand(data);
  }
  // Do not remove this log line. Flushes stale data from buffer.
  logInfo("received command: " + c.cmd);
  cmdRes = executeCommand(c);

  delay(DELAY_MS);
}

bool isEncryptedCommand(String cmd) {
  return cmd != COMMAND_PAIR &&
         cmd != COMMAND_CHECK_PAIRING &&
         cmd != COMMAND_PING;
}

bool isNotInternalCommand(String type) {
  return type != EVENT_INTERNAL_COMMAND;
}

bool isNotCommandEvent(String type) {
  return type != EVENT_SERIAL_DATA && type != EVENT_INTERNAL_COMMAND;
}


CommandResponse executeCommand(Command c) {
  if (c.cmd == COMMAND_PING)
    return executePing(c.data);

  if (c.cmd == COMMAND_CHECK_PAIRING)
    return executeCheckPairing(c.data);

  if (c.cmd == COMMAND_PAIR)
    return executePair(c.data);

  if (c.cmd == COMMAND_HELP)
    return executeHelp(c.data);

  if (c.cmd == COMMAND_WIPE)
    return executeWhipeHww(c.data);

  if (c.cmd == COMMAND_PASSWORD)
    return executePasswordCheck(c.data);

  if (c.cmd == COMMAND_PASSWORD_CLEAR)
    return executePasswordClear(c.data);

  if (c.cmd == COMMAND_ADDRESS)
    return executeShowAddress(c.data);

  if (c.cmd == COMMAND_SEED)
    return executeShowSeed(c.data);

  if (c.cmd == COMMAND_SEND_PSBT)
    return executeSignPsbt(c.data);

  if (c.cmd == COMMAND_RESTORE)
    return executeRestore(c.data);

  if (c.cmd == COMMAND_XPUB)
    return executeXpub(c.data);

  return executeUnknown(c.cmd + ": " + c.data);

}


HwwInitData initHww(String password, String mnemonic, String passphrase, bool persistSecrets) {
  if (isValidPassword(password) == false)
    return {"", "", false};

  if (mnemonic == "") {
    mnemonic = generateStrongerMnemonic(24); // todo: allow 12 also
  }

  String passwordHash  = hashStringData(password);

  if (persistSecrets == true) {
    deleteFile(SPIFFS, global.mnemonicFileName.c_str());
    deleteFile(SPIFFS, global.passwordFileName.c_str());
    writeFile(SPIFFS, global.passwordFileName.c_str(), passwordHash);

    int byteSize =  passwordHash.length() / 2;
    byte encryptionKey[byteSize];
    fromHex(passwordHash, encryptionKey, byteSize);

    writeFile(SPIFFS, global.mnemonicFileName.c_str(), encryptDataWithIv(encryptionKey, mnemonic));
  }

  return {passwordHash, mnemonic, true};
}

void sendCommandOutput(String command, String commandData) {
  commandOutToFile(command + " " + commandData);
  serialPrintlnSecure(command + " " + commandData);
}

void serialPrintlnSecure(String msg) {
  String encryptedHex = encryptDataWithIv(global.dhe_shared_secret, msg);
  Serial.println(encryptedHex);
}

void commandOutToFile(const String msg) {
  if (global.hasCommandsFile == true) {
    appendFile(SD, global.commandsOutFileName.c_str(), msg + "\n");
  }
}

Command decryptAndExtractCommand(String ecryptedData) {
  String data = decryptDataWithIv(global.dhe_shared_secret, ecryptedData);
  return extractCommand(data);
}

EventData toggleDatanAndQR(String data, bool showQR) {
  String dataUpper = data + "";
  dataUpper.toUpperCase();

  EventData event = {EVENT_BUTTON_ACTION, "0 1"};
  while (event.type == EVENT_BUTTON_ACTION) {
    String buttonState = getWordAtPosition(event.data, 1);

    if (buttonState == "1") {
      if (showQR == true) {
        showQRCode(dataUpper);
      } else {
        showMessage(data, "");
      }
    } else {
      showQR = !showQR;
    }
    event = awaitEvent();
  }
  return event;
}
