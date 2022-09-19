CommandResponse executeXpub(String commandData) {
  if (global.authenticated == false) {
    return {"Enter password!", "8 numbers/letters"};
  }

  int spacePos = commandData.indexOf(" ");
  String networkName = getWordAtPosition(commandData, 0);
  String path = getWordAtPosition(commandData, 1);

  const Network * network;
  if (networkName == "Mainnet") {
    network = &Mainnet;
  } else if (networkName == "Testnet") {
    network = &Testnet;
  } else {
    return {"Unknown Network",  "Must be Mainnet or Testnet"};
  }

  if (!path) {
    return {"Derivation path missing!", "XPUB not generated"};
  }

  HDPrivateKey hd(global.mnemonic, global.passphrase, network);
  if (!hd) {
    serialSendCommand(COMMAND_XPUB, "0 invalid_mnemonic");
    return {"Invalid Mnemonic", ""};
  }
  HDPrivateKey account = hd.derive(path);
  String xpub = account.xpub();
  serialSendCommand(COMMAND_XPUB, "1 " + xpub + " " + hd.fingerprint());

  bool showQR = true;

  String xpubUpper = xpub + "";
  xpubUpper.toUpperCase();
  

  EventData event = {EVENT_BUTTON_ACTION, ""};
  while (event.type == EVENT_BUTTON_ACTION) {
    if (showQR == true) {
      showQRCode(xpubUpper);
    } else {
      showMessage(xpub, "");
    }

    String buttonState = getWordAtPosition(event.data, 1);
    if (buttonState == "1") {
      showQR = !showQR;
    }
    event = awaitEvent();
  }

  return { "", "", 0, event };
}
