/**
   @brief Check if the device is already paired with the client.
   Skip pairing if the `shared secret` is valid.
   @param encryptedData: String. `PAIRING_CONTROL_TEXT` value encrypted by the client.
   @return CommandResponse
*/
CommandResponse executeCheckPairing(String encryptedData) {
  String sharedSecret = bytesToHexString(global.dhe_shared_secret, sizeof(global.dhe_shared_secret));

  if (sharedSecret.equals("0000000000000000000000000000000000000000000000000000000000000000")) {
    return {"Ready", "For encrypted connection"};
  }
  String data = decryptDataWithIv(global.dhe_shared_secret, encryptedData);
  if (data == PAIRING_CONTROL_TEXT) {
    String encryptedControlText = encryptDataWithIv(
      global.dhe_shared_secret,
      PAIRING_CONTROL_TEXT
    );
    if (encryptedControlText == "") {
      Serial.println(COMMAND_CHECK_PAIRING + " 1 rng_failure");
      return {"RNG failure", "Connection refused"};
    }
    Serial.println(COMMAND_CHECK_PAIRING + " 0 " + encryptedControlText);
    return {"Paired", "Encrypted connection"};
  }
  Serial.println(COMMAND_CHECK_PAIRING + " 1 must_repair");
  return {"Must re-pair", "Restart device"};
}
