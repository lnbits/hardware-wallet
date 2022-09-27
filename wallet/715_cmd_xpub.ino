/**
   @brief

   @param data: String. Space separated values. Use minus (`-`) to skip the value.
    Value significance by position:
     0 - networkName: String. Can be `Testnet` or `Mainnet`.
     1 - path: String. The BIP32 full for  the `xpub`. Eg: `m/84'/1'/0'`.
   @return CommandResponse
      - `xpub` QR or text to the UI.
      - `/xpub 0 invalid_mnemonic` to the client. If the stored mnemonic is invalid.
      - `/xpub 1 {xpub} {fingerprint}` to the client.
*/
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
    sendCommandOutput(COMMAND_XPUB, "0 invalid_mnemonic");
    return {"Invalid Mnemonic", ""};
  }
  HDPrivateKey account = hd.derive(path);
  String xpub = account.xpub();
  sendCommandOutput(COMMAND_XPUB, "1 " + xpub + " " + hd.fingerprint());

  EventData event = toggleDatanAndQR(xpub, true);

  return { "", "", 0, event };
}
