CommandResponse executeXpub(String commandData) {
  if (global.authenticated == false) {
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

  HDPrivateKey hd(global.mnemonic, global.passphrase, &Testnet);
  if (!hd) {
    serialSendCommand(COMMAND_XPUB, "0 invalid_mnemonic");
    return {"Invalid Mnemonic", ""};
  }
  HDPrivateKey account = hd.derive(path);
  String xpub = account.xpub();
  serialSendCommand(COMMAND_XPUB, "1 " + xpub + " " + hd.fingerprint());
  return { xpub, "" };
}