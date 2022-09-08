//========================================================================//
//================================COMMANDS================================//
//========================================================================//

CommandResponse cmdRes = {"Welcome", "Row, row, row your boat"};


void listenForCommands() {
  if (cmdRes.message != "" || cmdRes.subMessage != "")
    showMessage(cmdRes.message, cmdRes.subMessage);

  int currentState1 = digitalRead(global.button1Pin);
  int currentState2 = digitalRead(global.button2Pin);

  EventData event = awaitEvent();
  if (event.type != EVENT_SERIAL_DATA) return;

  String data = event.data;

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
  return cmd != COMMAND_PAIR &&
         cmd != COMMAND_CHECK_PAIRING &&
         cmd != COMMAND_PING;
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

// unsigned long lastTickTime = 0;
// int counter = 10;

int button1State = HIGH;
int button2State = HIGH;


EventData awaitEvent() {
  unsigned long  waitTime = millis();
  bool idle = true;
  while (Serial.available() == 0) {
    // check if ok for pairing or if idle
    if (idle == true) {
      if  ((millis() - waitTime) > 60 * 1000) {
        idle = false;
        logo(0);
      }
      // else if  (counter > 0 && ((millis() - lastTickTime) > 1000)) {
      //     counter--;
      //     lastTickTime = millis();
      //     logo(counter);
      //   } else if (counter == 0) {
      //     logo(counter);
      //     counter--;
      //   }
    }

    // button state
    int button1NewState = digitalRead(global.button1Pin);
    if (button1NewState != button1State) {
      logSerial("button 1: " + String(button1NewState));
      button1State = button1NewState;
      return { EVENT_BUTTON_ACTION, "", global.button1Pin, button1NewState };
    }

    int button2NewState = digitalRead(global.button2Pin);
    if (button2NewState != button2State) {
      logSerial("button 2: " + String(button2NewState));
      button2State = button2NewState;
      return { EVENT_BUTTON_ACTION, "", global.button2Pin, button2NewState };
    }
  }
  // counter = -1;
  String data = Serial.readStringUntil('\n');
  return { EVENT_SERIAL_DATA, data };
}

HwwInitData initHww(String password, String mnemonic, String passphrase) {
  if (isAlphaNumeric(password) == false)
    return {"", "", false};

  deleteFile(SPIFFS, global.mnemonicFileName.c_str());
  deleteFile(SPIFFS, global.passwordFileName.c_str());
  if (mnemonic == "") {
    mnemonic = generateStrongerMnemonic(24); // todo: allow 12 also
  }

  String passwordHash  = hashStringData(password);
  writeFile(SPIFFS, global.passwordFileName.c_str(), passwordHash);

  int byteSize =  passwordHash.length() / 2;
  byte encryptionKey[byteSize];
  fromHex(passwordHash, encryptionKey, byteSize);

  String mnemonicWithPassphrase = mnemonic;
  if (passphrase) {
    mnemonicWithPassphrase += "/" + passphrase;
  }

  writeFile(SPIFFS, global.mnemonicFileName.c_str(), encryptDataWithIv(encryptionKey, mnemonicWithPassphrase));

  return {passwordHash, mnemonic, true};
}

void serialSendCommand(String command, String commandData) {
  serialPrintlnSecure(command + " " + commandData);
}

void serialPrintlnSecure(String msg) {
  String encryptedHex = encryptDataWithIv(global.dhe_shared_secret, msg);
  Serial.println(encryptedHex);
}

Command decryptAndExtractCommand(String ecryptedData) {
  String data = decryptDataWithIv(global.dhe_shared_secret, ecryptedData);
  return extractCommand(data);
}
