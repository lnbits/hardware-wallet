//========================================================================//
//================================COMMANDS================================//
//========================================================================//

String encrytptedMnemonic = "";
String passwordHash = "";
byte dhe_secret[32];
byte dhe_shared_secret[32];

bool authenticated = false;
CommandResponse cmdRes = {"Welcome", "Row, row, row your boat"};

String command = ""; // todo: remove
String commandData = ""; // todo: remove


String serialData = "";

void listenForCommands() {
  // todo: called too many times
  if (loadFiles() == false) {
    cmdRes = { "Failed opening files",  "Reset or 'help'"};
  }

  if (cmdRes.message != "" || cmdRes.subMessage != "")
    showMessage(cmdRes.message, cmdRes.subMessage);

  serialData = awaitSerialData();

  Command c = extractCommand(serialData);
  if (c.cmd != COMMAND_DH_EXCHANGE) {
    serialData = decryptMessageWithIv(serialData);
    c = extractCommand(serialData);
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
    return executeWipeHww(c.data);

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

CommandResponse executeDhExchange(String publicKeyHex) {
  logSerial("### publicKeyHex: " + publicKeyHex);
  String tempMnemonic = createMnemonic(24);
  mnemonicToEntropy(tempMnemonic, dhe_secret, sizeof(dhe_secret));
  logSerial("### dhe_secret: " + toHex(dhe_secret, 32));

  PrivateKey dhPrivateKey(dhe_secret);
  PublicKey dhPublicKey = dhPrivateKey.publicKey();
  logSerial("### dhPublicKey: " + toHex(dhPublicKey.point, 64));

  byte publicKeyBin[64];
  fromHex(publicKeyHex, publicKeyBin, 64);
  PublicKey otherDhPublicKey(publicKeyBin, false);
  dhPrivateKey.ecdh(otherDhPublicKey, dhe_shared_secret, false);

  logSerial("### dhe_shared_secret: " + toHex(dhe_shared_secret, sizeof(dhe_shared_secret)));
  Serial.println(COMMAND_DH_EXCHANGE + " " + toHex(dhPublicKey.point, sizeof(dhPublicKey.point)));
  logSerial("sent: " + COMMAND_DH_EXCHANGE + " " + toHex(dhPublicKey.point, sizeof(dhPublicKey.point)));
  return {"Connected", "Encrypted connection"};
}

CommandResponse executeHelp(String commandData) {
  help();
  return {"More info at:", "github.com/lnbits/hardware-wallet"};
}

CommandResponse executePasswordCheck(String commandData) {
  if (commandData == "") {
    return { "Enter password",  "8 numbers/letters", 0 };
  }
  String hash = hashPassword(commandData);
  if (passwordHash == hash) {
    authenticated = true;
    serialSendCommand(COMMAND_PASSWORD, String(authenticated));
    return {"Password correct!",   "Ready to sign sir!" };
  }
  authenticated = false;
  return {"Wrong password, try again", "8 numbers/letters"};
}

CommandResponse executePasswordClear(String commandData) {
  authenticated = false;
  serialSendCommand(COMMAND_PASSWORD_CLEAR, "1");
  showMessage("Logging out...", "");
  delay(2000);

  return {"Logged out",  "Enter password"};
}

CommandResponse executeWipeHww(String password) {
  if (password == "") {
    return { "Enter new password", "8 numbers/letters"};
  }

  showMessage("Resetting...", "");
  delay(2000);

  authenticated = wipeHww(password, "");
  serialSendCommand(COMMAND_WIPE,  String(authenticated));
  if (authenticated == true) {
    return { "Successfully wiped!",  "Every new beginning comes from some other beginning's end."};
  }
  return {"Error, try again"  "8 numbers/letters"};


}

CommandResponse executeShowSeed(String position) {
  if (authenticated == false) {
    return {"Enter password!", "8 numbers/letters"};
  }
  if (!position || position.toInt() == 0) {
    return {"Bad word position",  "`/help` for details " };
  }
  String word = getWordAtPosition(encrytptedMnemonic, position.toInt());
  printMnemonicWord(position, word);
  serialPrintlnSecure(word);
  return {"", ""};
}

CommandResponse executeXpub(String commandData) {
  if (authenticated == false) {
    return {"Enter password!", "8 numbers/letters"};
  }

  int spacePos = commandData.indexOf(" ");
  String networkName = commandData.substring(0, spacePos);
  String path = commandData.substring(spacePos + 1, commandData.length() );

  logSerial("xpub received: " + networkName + " path:" + path);

  const Network * network;
  if (networkName == "Mainnet") {
    network = &Mainnet;
  } else if (networkName == "Testnet") {
    network = &Testnet;
  } else {
    return {"Unknown Network",  "Must be Mainent or Testnet"};
  }

  if (!path) {
    return {"Derivation path missing!", "XPUB not generated"};
  }

  HDPrivateKey hd(encrytptedMnemonic, "", &Testnet);
  if (!hd) {
    serialSendCommand(COMMAND_XPUB, "0 invalid_mnemonic");
    return {"Invalid Mnemonic", ""};
  }
  HDPrivateKey account = hd.derive(path);
  String xpub = account.xpub();
  serialSendCommand(COMMAND_XPUB, "1 " + xpub + " " + hd.fingerprint());
  return { xpub, "" };
}

CommandResponse executeRestore(String mnemonic, String password) {
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
    serialData = awaitSerialData();
    Command c = extractCommand(serialData);

    if (c.cmd != COMMAND_PASSWORD)
      return executeUnknown("");

    password = c.data;
  }

  authenticated = wipeHww(password, mnemonic);
  serialSendCommand(COMMAND_RESTORE, String(authenticated));
  if (authenticated == true) {
    return {"Restore successfull",  "/seed` to view word list"};
  }
  return { "Error, try again", "8 numbers/letters"};
}

CommandResponse executeSignPsbt(String commandData) {
  if (authenticated == false) {
    return { "Enter password!", "8 numbers/letters"};
  }

  showMessage("Please wait", "Parsing PSBT...");
  int spacePos = commandData.indexOf(" ");
  String networkName = commandData.substring(0, spacePos);
  String psbtBase64 = commandData.substring(spacePos + 1, commandData.length() );

  const Network * network;
  if (networkName == "Mainnet") {
    network = &Mainnet;
  } else if (networkName == "Testnet") {
    network = &Testnet;
  } else {
    return { "Unknown Network", "Must be Mainent or Testnet"};
  }

  PSBT psbt = parseBase64Psbt(psbtBase64);
  if (!psbt) {
    logSerial("Failed psbt: " + psbtBase64);
    serialSendCommand(COMMAND_SEND_PSBT, "psbt_parse_failed");
    return {"Failed parsing",  "Send PSBT again"};
  }

  HDPrivateKey hd(encrytptedMnemonic, "", network); // todo: no passphrase yet
  // check if it is valid
  if (!hd) {
    serialSendCommand(COMMAND_SEND_PSBT, "invalid_mnemonic'");
    return {"Invalid Mnemonic", ""};
  }

  serialSendCommand(COMMAND_SEND_PSBT, "1");

  for (int i = 0; i < psbt.tx.outputsNumber; i++) {
    printOutputDetails(psbt, hd, i, network);
    serialData = awaitSerialData();
    Command c = extractCommand(serialData);
    if (c.cmd == COMMAND_CANCEL) {
      return {"Operation Canceled", "`/help` for details" };
    }
    if (c.cmd != COMMAND_CONFIRM_NEXT) {
      return executeUnknown(c.data);
    }
  }

  printFeeDetails(psbt.fee());

  serialData = awaitSerialData();
  Command c = extractCommand(serialData);
  if (c.cmd == COMMAND_SIGN_PSBT) {
    showMessage("Please wait", "Signing PSBT...");

    uint8_t signedInputCount = psbt.sign(hd);

    serialSendCommand(COMMAND_SIGN_PSBT, signedInputCount + " " + psbt.toBase64());
    return { "Signed inputs:", String(signedInputCount) };
  }
  if (c.cmd = COMMAND_CANCEL) {
    return { "Operation Canceled",  "`/help` for details" };
  }
  return  executeUnknown(commandData);
}

CommandResponse executeUnknown(String commandData) {
  return {"Unknown command",  "`/help` for details"};
}


bool loadFiles() {
  FileData mnFile = readFile(SPIFFS, "/mn.txt");
  encrytptedMnemonic = mnFile.data;

  FileData pwdFile = readFile(SPIFFS, "/hash.txt");
  passwordHash = pwdFile.data;

  return mnFile.success && pwdFile.success;
}

bool wipeHww(String password, String mnemonic) {
  if (isAlphaNumeric(password) == false)
    return false;

  deleteFile(SPIFFS, "/mn.txt");
  deleteFile(SPIFFS, "/hash.txt");
  if (mnemonic == "") {
    mnemonic = createMnemonic(24); // todo: allow 12 also
  }
  passwordHash = hashPassword(password);
  writeFile(SPIFFS, "/mn.txt", mnemonic);
  writeFile(SPIFFS, "/hash.txt", passwordHash);

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

  String messageHex = encryptData(dhe_shared_secret, ivHex, data);

  Serial.println(messageHex + ivHex);
}


String decryptMessageWithIv(String messageWithIvHex) {
  int ivSize = 16;
  String messageHex = messageWithIvHex.substring(0, messageWithIvHex.length() - ivSize * 2);
  String ivHex = messageWithIvHex.substring(messageWithIvHex.length() - ivSize * 2, messageWithIvHex.length());

  String decryptedData = decryptData(dhe_shared_secret, ivHex, messageHex);

  Command c = extractCommand(decryptedData);
  int commandLength = c.cmd.toInt();
  return c.data.substring(0, commandLength);
}

Command extractCommand(String s) {
  int spacePos = s.indexOf(" ");
  command = s.substring(0, spacePos);
  if (spacePos == -1) {
    commandData = "";
  } else {
    commandData = s.substring(spacePos + 1, s.length());
  }
  return {command, commandData};
}
