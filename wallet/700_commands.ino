//========================================================================//
//================================COMMANDS================================//
//========================================================================//

String encrytptedMnemonic = "";
String passwordHash = "";
byte dhe_secret[32];
byte dhe_shared_secret[32];

bool authenticated = false;
String command = ""; // todo: remove
String commandData = ""; // todo: remove

String message = "Welcome";
String subMessage = "Enter password";

String serialData = "";

void listenForCommands() {
  // todo: called too many times
  if (loadFiles() == false) {
    message = "Failed opening files";
    subMessage = "Reset or 'help'";
  }

  if (message != "" || subMessage != "")
    showMessage(message, subMessage);

  serialData = awaitSerialData();

  Command c = extractCommand(serialData);
  if (c.cmd != COMMAND_DH_EXCHANGE) {
    serialData = decryptData(serialData);
    c = extractCommand(serialData);
  }
  // flush stale data from buffer
  logSerial("received command: " + c.cmd);
  executeCommand(c);

  delay(DELAY_MS);
}


void executeCommand(Command c) {
  if (c.cmd == COMMAND_DH_EXCHANGE) {
    executeDhExchange(c.data);
  } else if (c.cmd == COMMAND_HELP) {
    executeHelp(c.data);
  } else if (c.cmd == COMMAND_WIPE) {
    executeWipeHww(c.data);
  } else if (c.cmd == COMMAND_PASSWORD) {
    executePasswordCheck(c.data);
  } else if (c.cmd == COMMAND_PASSWORD_CLEAR) {
    executePasswordClear(c.data);
  } else if (c.cmd == COMMAND_SEED) {
    executeShowSeed(c.data);
  } else if (c.cmd == COMMAND_SEND_PSBT) {
    executeSignPsbt(c.data);
  } else if (c.cmd == COMMAND_RESTORE) {
    executeRestore(c.data, "");
  } else if (c.cmd == COMMAND_XPUB) {
    executeXpub(c.data);
  } else {
    executeUnknown(c.data);
  }
}
void executeDhExchange(String publicKeyHex) {
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
}
void executeHelp(String commandData) {

  String messageHex = commandData;
  int byteSize =  messageHex.length() / 2;
  byte messageBin[byteSize];

  logSerial("### messageHex: " + messageHex);
  logSerial("### byteSize: " + String(byteSize));

  fromHex(messageHex, messageBin, byteSize);
  // logSerial("### messageText return0: " + toHex(messageBin, sizeof(messageBin)));

  uint8_t iv[]  = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };
  logSerial("### iv: " + toHex(iv, sizeof(iv)));
  byte shared_secret[32];
  fromHex("f96c85875055a5586688fea4cf7c4a2bd9541ffcf34f9d663d97e0cf2f6af4af", shared_secret, 32);
  logSerial("### shared_secret: " + toHex(shared_secret, sizeof(shared_secret)));


  AES_ctx ctx;
  AES_init_ctx_iv(&ctx, shared_secret, iv);
  AES_CBC_decrypt_buffer(&ctx, messageBin, sizeof(messageBin));

  logSerial("### messageBin decrypt: " + toHex(messageBin, sizeof(messageBin)));

  String commandTxt = String((char *)messageBin).substring(0, byteSize);
  commandTxt.trim();
  logSerial("### messageText return2: " + commandTxt + ":");


  logSerial("### end help 1");
}

void executePasswordCheck(String commandData) {
  if (commandData == "") {
    message = "Enter password";
    subMessage = "8 numbers/letters";
    return;
  }
  String hash = hashPassword(commandData);
  if (passwordHash == hash) {
    authenticated = true;
    message = "Password correct!";
    subMessage = "Ready to sign sir!";
  } else {
    authenticated = false;
    message = "Wrong password, try again";
    subMessage = "8 numbers/letters";
  }
  Serial.println(COMMAND_PASSWORD + " " + String(authenticated));
}

void executePasswordClear(String commandData) {
  authenticated = false;
  Serial.println(COMMAND_PASSWORD_CLEAR + " 1");
  showMessage("Logging out...", "");
  delay(2000);

  message = "Logged out";
  subMessage = "Enter password";
}

void executeWipeHww(String password) {
  if (password == "") {
    message = "Enter new password";
    subMessage = "8 numbers/letters";
    return;
  }

  showMessage("Resetting...", "");
  delay(2000);

  authenticated = wipeHww(password, "");
  if (authenticated == true) {
    message = "Successfully wiped!";
    subMessage = "Every new beginning comes from some other beginning's end.";
  } else {
    message = "Error, try again";
    subMessage = "8 numbers/letters";
  }
  Serial.println(COMMAND_WIPE + " " + String(authenticated));
}

void executeShowSeed(String position) {
  if (authenticated == false) {
    message = "Enter password!";
    subMessage = "8 numbers/letters";
    return;
  }
  if (!position || position.toInt() == 0) {
    message = "Bad word position";
    subMessage = "`/help` for details ";
    return;
  }
  message = "";
  subMessage = "";
  String word = getWordAtPosition(encrytptedMnemonic, position.toInt());
  printMnemonicWord(position, word);
  serialPrintlnSecure(word + "XYZ0123456789abcdefxXx");
}

void executeXpub(String commandData) {
  if (authenticated == false) {
    message = "Enter password!";
    subMessage = "8 numbers/letters";
    return;
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
    message = "Unknown Network";
    subMessage = "Must be Mainent or Testnet";
    return;
  }

  if (!path) {
    message = "Derivation path missing!";
    subMessage = "XPUB not generated";
    return;
  }

  HDPrivateKey hd(encrytptedMnemonic, "", &Testnet);
  if (!hd) {
    message = "Invalid Mnemonic";
    Serial.println(COMMAND_XPUB + " 0 invalid_mnemonic");
    return;
  }
  HDPrivateKey account = hd.derive(path);
  String xpub = account.xpub();
  Serial.println(COMMAND_XPUB + " 1 " + xpub + " " + hd.fingerprint());
  message = xpub;
}

void executeRestore(String mnemonic, String password) {
  if (mnemonic == "") {
    message = "Enter seed words";
    subMessage = "Separated by spaces";
    return;
  }

  int size = getMnemonicBytes(mnemonic);
  if (size == 0) {
    message = "Wrong word count!";
    subMessage = "Must be 12, 15, 18, 21 or 24";
    Serial.println(COMMAND_RESTORE + " 0");
    return;
  }

  if (!hasValidChecksum(mnemonic, size)) {
    message = "Wrong mnemonic!";
    subMessage = "Incorrect checksum";
    Serial.println(COMMAND_RESTORE + " 0");
    return;
  }

  if (password == "") {
    showMessage("Enter new password!", "8 numbers/letters");
    serialData = awaitSerialData();
    Command c = extractCommand(serialData);
    if (c.cmd != COMMAND_PASSWORD) {
      executeUnknown("");
      return;
    }
    password = c.data;
  }

  authenticated = wipeHww(password, mnemonic);
  if (authenticated == true) {
    message = "Restore successfull";
    subMessage = "/seed` to view word list";
  } else {
    message = "Error, try again";
    subMessage = "8 numbers/letters";
  }
  Serial.println(COMMAND_RESTORE + " " + String(authenticated));
}

void executeSignPsbt(String commandData) {
  if (authenticated == false) {
    message = "Enter password!";
    subMessage = "8 numbers/letters";
    return;
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
    message = "Unknown Network";
    subMessage = "Must be Mainent or Testnet";
    return;
  }

  PSBT psbt = parseBase64Psbt(psbtBase64);
  if (!psbt) {
    logSerial("Failed psbt: " + psbtBase64);
    message = "Failed parsing";
    subMessage = "Send PSBT again";
    Serial.println(COMMAND_SEND_PSBT + " psbt_parse_failed");
    return;
  }

  HDPrivateKey hd(encrytptedMnemonic, "", network); // todo: no passphrase yet
  // check if it is valid
  if (!hd) {
    message = "Invalid Mnemonic";
    Serial.println(COMMAND_SEND_PSBT + " invalid_mnemonic'");
    return;
  }

  Serial.println(COMMAND_SEND_PSBT + " 1");

  for (int i = 0; i < psbt.tx.outputsNumber; i++) {
    printOutputDetails(psbt, hd, i, network);
    serialData = awaitSerialData();
    Command c = extractCommand(serialData);
    if (c.cmd == COMMAND_CANCEL) {
      message = "Operation Canceled";
      subMessage = "`/help` for details";
      return;
    } else if (c.cmd != COMMAND_CONFIRM_NEXT) {
      executeUnknown(c.data);
      return;
    }
  }
  printFeeDetails(psbt.fee());


  serialData = awaitSerialData();
  Command c = extractCommand(serialData);
  if (c.cmd == COMMAND_SIGN_PSBT) {
    showMessage("Please wait", "Signing PSBT...");

    uint8_t signedInputCount = psbt.sign(hd);

    Serial.println(COMMAND_SIGN_PSBT + " " + signedInputCount + " " + psbt.toBase64());
    message = "Signed inputs:";
    subMessage = String(signedInputCount);
  } else if (c.cmd = COMMAND_CANCEL) {
    message = "Operation Canceled";
    subMessage = "`/help` for details";
  } else {
    executeUnknown(commandData);
  }
}

void executeUnknown(String commandData) {
  message = "Unknown command";
  subMessage = "`/help` for details";
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
  // todo: use checkMnemonic (Bitcoin.cpp)
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


bool hasValidChecksum(String mnemonic, int size) {
  uint8_t out[size];
  size_t len = mnemonicToEntropy(mnemonic, out, sizeof(out));
  String deserializedMnemonic = mnemonicFromEntropy(out, sizeof(out));
  return mnemonic == deserializedMnemonic;
}

void serialPrintlnSecure(String msg) {
  String data = String(msg.length()) + " " + msg;
  while (data.length() % 16 != 0) data += " ";

  String messageHex = encryptData(data);
  logSerial(messageHex);
}


String encryptData(String msg) {
  uint8_t iv[]  = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };

  byte messageBin[msg.length()];
  msg.getBytes(messageBin, msg.length());

  // logSerial("### encryptData msg:"+ msg + ":"+String(sizeof(messageBin)));
  // logSerial("### encryptData messageBin: " + toHex(messageBin, sizeof(messageBin)));

  AES_ctx ctx;
  AES_init_ctx_iv(&ctx, dhe_shared_secret, iv);

  AES_CBC_encrypt_buffer(&ctx, messageBin, sizeof(messageBin));
  // logSerial("### messageHex: " + toHex(messageBin, sizeof(messageBin)));

  return toHex(messageBin, sizeof(messageBin));
}

String decryptData(String messageHex) {
  int byteSize =  messageHex.length() / 2;
  byte messageBin[byteSize];
  fromHex(messageHex, messageBin, byteSize);

  uint8_t iv[]  = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };
  AES_ctx ctx;
  AES_init_ctx_iv(&ctx, dhe_shared_secret, iv);
  AES_CBC_decrypt_buffer(&ctx, messageBin, sizeof(messageBin));
  String commandTxt = String((char *)messageBin).substring(0, byteSize);

  // logSerial("### decryptData messageHex:"+messageHex);
  // logSerial("### decryptData commandTxt:"+commandTxt);
  // not actually a command
  Command c = extractCommand(commandTxt);
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
