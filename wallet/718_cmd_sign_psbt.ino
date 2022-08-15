CommandResponse executeSignPsbt(String commandData) {
  if (g.authenticated == false) {
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

  HDPrivateKey hd(g.encrytptedMnemonic, "", network); // todo: no passphrase yet
  // check if it is valid
  if (!hd) {
    serialSendCommand(COMMAND_SEND_PSBT, "invalid_mnemonic'");
    return {"Invalid Mnemonic", ""};
  }

  serialSendCommand(COMMAND_SEND_PSBT, "1");

  for (int i = 0; i < psbt.tx.outputsNumber; i++) {
    printOutputDetails(psbt, hd, i, network);
    String data = awaitSerialData();
    Command c = extractCommand(data);
    if (c.cmd == COMMAND_CANCEL) {
      return {"Operation Canceled", "`/help` for details" };
    }
    if (c.cmd != COMMAND_CONFIRM_NEXT) {
      return executeUnknown(c.data);
    }
  }

  printFeeDetails(psbt.fee());

  String data = awaitSerialData();
  Command c = extractCommand(data);
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