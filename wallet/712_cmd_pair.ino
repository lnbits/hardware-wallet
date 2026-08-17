/**
   @brief Create an encripted connection between the device and the client.
   It uses `Diffie-Hellman key exchange` to build a `shared secret`.
   It uses Advanced Encryption Standard (AES) symetric encryption with the `shared secret` as key.
   In this command certain configs can be initialized.
   @param data: String. Space separated values. Use minus (`-`) to skip the value.
    Value significance by position:
    0 - publicKeyHex: String (optional). The public key of the client in hex format, used in DHKE.
    1 - button1Pin: legacy field, accepted but ignored. Pins come from the board profile.
    2 - button2Pin: legacy field, accepted but ignored. Pins come from the board profile.
    3 - persistSecrets: boolean (optional).
          - Persist the password-protected wallet record when restoring a wallet.
          - Set to `false` in air-gapped mode when the seed is provided in `commands.in.txt`
          - Defaults to `true`

   @return CommandResponse
*/
CommandResponse executePair(String data) {
  if ((millis() - global.startTime) > 10 * 1000) {
    Serial.println(COMMAND_PAIR + " 1 " + " connection_period_expired");
    return {"Connection Refused", "10 secs from reboot passed"};
  }

  // A legacy client needs 128 hex characters plus three short positional
  // options. Refuse oversized unauthenticated frames before token parsing.
  if (data.length() > 192) {
    Serial.println(COMMAND_PAIR + " 1 invalid_request");
    return {"Pairing refused", "Request too long"};
  }

  String publicKeyHex = getWordAtPosition(data, 0);
  String button1Pin = getWordAtPosition(data, 1);
  String button2Pin = getWordAtPosition(data, 2);
  String persistSecrets = getWordAtPosition(data, 3);

  if (isNotEmptyParam(publicKeyHex)) {
    return pair(publicKeyHex, button1Pin, button2Pin, persistSecrets);
  }

  if (persistSecrets == "false") global.persistSecrets = false;

  return {"Pairing", "Not encrypted"};
}

CommandResponse pair(
  String publicKeyHex,
  String button1Pin,
  String button2Pin,
  String persistSecrets
) {
  if (publicKeyHex.length() != 128 || isStrictHex(publicKeyHex) == false) {
    Serial.println(COMMAND_PAIR + " 1 invalid_public_key");
    return {"Pairing refused", "Invalid public key"};
  }

  byte dhe_secret[32] = {0};
  if (!deriveHealthyHardwareEntropy(dhe_secret, sizeof(dhe_secret))) {
    clearSensitiveBytes(dhe_secret, sizeof(dhe_secret));
    Serial.println(COMMAND_PAIR + " 1 rng_failure");
    return {"RNG failure", "Pairing aborted"};
  }

  uint8_t compressedPublicKey[EC_PUBLIC_KEY_LEN] = {0};
  uint8_t publicKey[EC_PUBLIC_KEY_UNCOMPRESSED_LEN] = {0};
  if (wally_ec_public_key_from_private_key(
        dhe_secret, sizeof(dhe_secret), compressedPublicKey, sizeof(compressedPublicKey)
      ) != WALLY_OK ||
      wally_ec_public_key_decompress(
        compressedPublicKey, sizeof(compressedPublicKey), publicKey, sizeof(publicKey)
      ) != WALLY_OK) {
    clearSensitiveBytes(dhe_secret, sizeof(dhe_secret));
    clearSensitiveBytes(compressedPublicKey, sizeof(compressedPublicKey));
    clearSensitiveBytes(publicKey, sizeof(publicKey));
    Serial.println(COMMAND_PAIR + " 1 rng_failure");
    return {"RNG failure", "Pairing aborted"};
  }

  byte publicKeyBin[EC_PUBLIC_KEY_UNCOMPRESSED_LEN] = {0};
  publicKeyBin[0] = 0x04;
  if (!hexStringToBytes(publicKeyHex, publicKeyBin + 1, sizeof(publicKeyBin) - 1)) {
    clearSensitiveBytes(publicKeyBin, sizeof(publicKeyBin));
    clearSensitiveBytes(dhe_secret, sizeof(dhe_secret));
    clearSensitiveBytes(compressedPublicKey, sizeof(compressedPublicKey));
    clearSensitiveBytes(publicKey, sizeof(publicKey));
    Serial.println(COMMAND_PAIR + " 1 invalid_public_key");
    return {"Pairing refused", "Invalid public key"};
  }
  if (!legacyCompatibleEcdh(
        dhe_secret, publicKeyBin, global.dhe_shared_secret
      )) {
    clearSensitiveBytes(publicKeyBin, sizeof(publicKeyBin));
    clearSensitiveBytes(dhe_secret, sizeof(dhe_secret));
    clearSensitiveBytes(compressedPublicKey, sizeof(compressedPublicKey));
    clearSensitiveBytes(publicKey, sizeof(publicKey));
    Serial.println(COMMAND_PAIR + " 1 invalid_public_key");
    return {"Pairing refused", "Invalid public key"};
  }
  clearSensitiveBytes(publicKeyBin, sizeof(publicKeyBin));
  clearSensitiveBytes(dhe_secret, sizeof(dhe_secret));
  clearSensitiveBytes(compressedPublicKey, sizeof(compressedPublicKey));

  String devicePublicKeyHex = bytesToHexString(publicKey + 1, sizeof(publicKey) - 1);
  clearSensitiveBytes(publicKey, sizeof(publicKey));
  String sharedSecretHex = bytesToHexString(global.dhe_shared_secret, sizeof(global.dhe_shared_secret));
  if (devicePublicKeyHex == "" || sharedSecretHex == "") {
    clearSensitiveBytes(global.dhe_shared_secret, sizeof(global.dhe_shared_secret));
    Serial.println(COMMAND_PAIR + " 1 crypto_failure");
    return {"Pairing failed", "Crypto memory error"};
  }
  Serial.println(COMMAND_PAIR + " 0 " + devicePublicKeyHex);

  // Do not let a malformed unauthenticated request mutate session policy.
  if (persistSecrets == "false") global.persistSecrets = false;
  updateDeviceConfig(button1Pin, button2Pin);

  String fingerprint = hashStringData(sharedSecretHex).substring(0, 5);
  fingerprint.toUpperCase();
  return {"Confirm", "Code: " + fingerprint };
}

void updateDeviceConfig(String button1Pin, String button2Pin) {
  // Retain the legacy positional parameters for client compatibility, but do
  // not let an unauthenticated pairing frame reassign hardware GPIOs.
  if (isNotEmptyParam(button1Pin) || isNotEmptyParam(button2Pin)) {
    logInfo("Pairing button pin overrides ignored for board " + String(BOARD.id));
  }
  global.button1Pin = BOARD.primaryButtonPin;
  global.button2Pin = BOARD.secondaryButtonPin;
  writeFile(SPIFFS, global.deviceMetaFileName.c_str(), global.deviceId);
}
