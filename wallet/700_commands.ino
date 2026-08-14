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

  if (c.cmd == COMMAND_CREATE)
    return executeCreate(c.data);

  if (c.cmd == COMMAND_XPUB)
    return executeXpub(c.data);

  // Never echo decrypted command arguments into serial or SD diagnostics.
  // A typo in a password/restore command must not turn its secret into a log.
  return executeUnknown(c.cmd);

}


HwwInitData initHww(String password, String mnemonic, String passphrase, bool persistSecrets) {
  if (isValidPassword(password) == false)
    return {"", "", false};

  if (mnemonic == "") {
    mnemonic = generateStrongerMnemonic(24); // todo: allow 12 also
    if (mnemonic == "") return {"", "", false};
  }

  if (!initializeProtectedWallet(password, mnemonic, persistSecrets)) {
    mnemonic = "";
    return {"", "", false};
  }

  return {global.passwordVerifier, mnemonic, true};
}

void sendCommandOutput(String command, String commandData) {
  commandOutToFile(command + " " + commandData);
  serialPrintlnSecure(command + " " + commandData);
}

void serialPrintlnSecure(String msg) {
  String encryptedHex = encryptDataWithIv(global.dhe_shared_secret, msg);
  if (encryptedHex == "") {
    logInfo("Secure output aborted: RNG health check failed");
    return;
  }
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

  if (showQR == true) {
    showQRCode(dataUpper);
  } else {
    showMessage(data, "");
    setUiControls(UiControls::ToggleOnly);
  }

  armInputForPrompt();
  EventData event = awaitEvent();
  while (event.type == EVENT_BUTTON_ACTION) {
    showQR = !showQR;
    if (showQR == true) {
      showQRCode(dataUpper);
    } else {
      showMessage(data, "");
      setUiControls(UiControls::ToggleOnly);
    }
    event = awaitEvent();
  }
  return event;
}
