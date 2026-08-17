/**
   @brief Show an address text and QR Code based on the BIP32 path.
   It compares the derived address with the one received from the client.
   If they are different an warning message will be shown.
   @param addressData: String. Space separated values. Use minus (`-`) to skip the value.
    Value significance by position:
    0 - networkName: String. Can be `Testnet` or `Mainnet`.
    1 - path: String. BIP32 (full path). Eg:
    2 - address: String. Address derviced by the client. Used for validation
   @return CommandResponse
*/
CommandResponse executeShowAddress(String addressData) {
  if (global.authenticated == false) {
    return {"Enter password!", "8 numbers/letters"};
  }

  String networkName = getWordAtPosition(addressData, 0);
  String path = getWordAtPosition(addressData, 1);
  String address = getWordAtPosition(addressData, 2);


  bool mainnet = false;
  if (!isMainnetName(networkName, &mainnet)) {
    return {"Unknown Network",  "Must be Mainnet or Testnet"};
  }

  if (isEmptyParam(path)) {
    return {"Derivation path missing!", "Address cannot be derived"};
  }
  if (!standardPathMatchesNetwork(path, mainnet)) {
    return {"Invalid path", "Purpose/network mismatch"};
  }

  struct ext_key root = {};
  struct ext_key derived = {};
  if (!mnemonicIsValid(global.mnemonic) || !deriveRootKey(mainnet, &root)) {
    return {"Invalid mnemonic", "Address cannot be derived"};
  }
  if (!derivePathKey(&root, path, &derived)) {
    clearSensitiveBytes((uint8_t *)&root, sizeof(root));
    return {"Invalid path", "Address cannot be derived"};
  }
  String derivedAddress = addressForDerivedKey(&derived, path, mainnet);
  clearSensitiveBytes((uint8_t *)&derived, sizeof(derived));
  clearSensitiveBytes((uint8_t *)&root, sizeof(root));
  if (derivedAddress == "") {
    return {"Unsupported path", "Use BIP44, BIP49 or BIP84"};
  }

  if (isEmptyParam(address)) {
    logInfo("Address cannot be validated. Path: " + path + " address: " + derivedAddress );
  } else if (!derivedAddress.equals(address)) {
    return {"Danger! Address missmatch!", "Derived address different thant the UI address"};
  }

  sendCommandOutput(COMMAND_ADDRESS, "1 " + derivedAddress);

  EventData event = toggleDatanAndQR(derivedAddress, false);

  return {"", "", 0, event};
}
