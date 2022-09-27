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

  const Network * network;
  if (networkName == "Mainnet") {
    network = &Mainnet;
  } else if (networkName == "Testnet") {
    network = &Testnet;
  } else {
    return {"Unknown Network",  "Must be Mainnet or Testnet"};
  }

  if (isEmptyParam(path)) {
    return {"Derivation path missing!", "Address cannot be derived"};
  }

  HDPrivateKey hd(global.mnemonic, global.passphrase, network);

  HDPrivateKey pK = hd.derive(path);
  String derivedAddress = pK.address();
  if (!derivedAddress.equals(address)) {
    return {"Danger! Address missmatch!", "Derived address different thant the UI address"};
  }

  EventData event = toggleDatanAndQR(derivedAddress, false);

  return {"", "", 0, event};
}
