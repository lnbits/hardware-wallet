/**
   @brief

   @param commandData: String. Space separated values. If empty, defaults to
    `Mainnet m/84'/0'/0'`.
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

  String networkName = getWordAtPosition(commandData, 0);
  String path = getWordAtPosition(commandData, 1);

  if (commandData.length() == 0) {
    networkName = "Mainnet";
    path = "m/84'/0'/0'";
  }

  bool mainnet = false;
  if (!isMainnetName(networkName, &mainnet)) {
    return {"Unknown Network",  "Must be Mainnet or Testnet"};
  }

  if (!path) {
    return {"Derivation path missing!", "XPUB not generated"};
  }
  if (!standardPathMatchesNetwork(path, mainnet)) {
    return {"Invalid path", "Purpose/network mismatch"};
  }

  struct ext_key root = {};
  struct ext_key account = {};
  if (!mnemonicIsValid(global.mnemonic) || !deriveRootKey(mainnet, &root)) {
    sendCommandOutput(COMMAND_XPUB, "0 invalid_mnemonic");
    return {"Invalid Mnemonic", ""};
  }
  if (!derivePathKey(&root, path, &account)) {
    clearSensitiveBytes((uint8_t *)&root, sizeof(root));
    return {"Invalid path", "XPUB not generated"};
  }
  String xpub = serializeAccountPublicKey(&account, mainnet, path);
  String fingerprint = rootFingerprint(&root);
  clearSensitiveBytes((uint8_t *)&account, sizeof(account));
  clearSensitiveBytes((uint8_t *)&root, sizeof(root));
  if (xpub == "" || fingerprint == "") {
    return {"XPUB failed", "Could not serialize key"};
  }
  sendCommandOutput(COMMAND_XPUB, "1 " + xpub + " " + fingerprint);

  EventData event = toggleDatanAndQR(xpub, true);

  return { "", "", 0, event };
}
