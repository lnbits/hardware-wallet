/**
   @brief Create an encripted connection between the device and the client.
   It uses `Diffie-Hellman key exchange` to build a `shared secret`.
   It uses Advanced Encryption Standard (AES) symetric encryption with the `shared secret` as key.
   In this command certain configs can be initialized.
   @param data: String. Space separated values. Use minus (`-`) to skip the value.
    Value significance by position:
    0 - publicKeyHex: String (optional). The public key of the client in hex format, used in DHKE.
    1 - button1Pin: int (optional). The pin number for the first button (optional).
    2 - button2Pin: int (optional). The pin number for the second button (optional).
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

  String tempMnemonic = generateStrongerMnemonic(24);
  byte dhe_secret[32] = {0};
  if (
    tempMnemonic == "" ||
    mnemonicToEntropy(tempMnemonic, dhe_secret, sizeof(dhe_secret)) != sizeof(dhe_secret)
  ) {
    clearSensitiveBytes(dhe_secret, sizeof(dhe_secret));
    Serial.println(COMMAND_PAIR + " 1 rng_failure");
    return {"RNG failure", "Pairing aborted"};
  }

  PrivateKey dhPrivateKey(dhe_secret);
  clearSensitiveBytes(dhe_secret, sizeof(dhe_secret));
  if (!dhPrivateKey) {
    Serial.println(COMMAND_PAIR + " 1 rng_failure");
    return {"RNG failure", "Pairing aborted"};
  }
  PublicKey dhPublicKey = dhPrivateKey.publicKey();

  byte publicKeyBin[64] = {0};
  if (fromHex(publicKeyHex, publicKeyBin, sizeof(publicKeyBin)) != sizeof(publicKeyBin)) {
    clearSensitiveBytes(publicKeyBin, sizeof(publicKeyBin));
    Serial.println(COMMAND_PAIR + " 1 invalid_public_key");
    return {"Pairing refused", "Invalid public key"};
  }
  PublicKey otherDhPublicKey(publicKeyBin, false);
  if (!otherDhPublicKey) {
    clearSensitiveBytes(publicKeyBin, sizeof(publicKeyBin));
    Serial.println(COMMAND_PAIR + " 1 invalid_public_key");
    return {"Pairing refused", "Invalid public key"};
  }
  dhPrivateKey.ecdh(otherDhPublicKey, global.dhe_shared_secret, false);
  clearSensitiveBytes(publicKeyBin, sizeof(publicKeyBin));

  Serial.println(COMMAND_PAIR + " 0 " + toHex(dhPublicKey.point, sizeof(dhPublicKey.point)));

  String sharedSecretHex =  toHex(global.dhe_shared_secret, sizeof(global.dhe_shared_secret));

  // Do not let a malformed unauthenticated request mutate session policy.
  if (persistSecrets == "false") global.persistSecrets = false;
  updateDeviceConfig(button1Pin, button2Pin);

  String fingerprint = hashStringData(sharedSecretHex).substring(0, 5);
  fingerprint.toUpperCase();
  return {"Confirm", "Code: " + fingerprint };
}

void updateDeviceConfig(String button1Pin, String button2Pin) {
  String deviceConfig = global.deviceId;
  if (isNotEmptyParam(button1Pin)) {
    global.button1Pin = button1Pin.toInt();
    deviceConfig = deviceConfig + " " + button1Pin;
  }

  if (isNotEmptyParam(button2Pin)) {
    global.button2Pin = button2Pin.toInt();
    deviceConfig = deviceConfig + " " + button2Pin;
  }

  // update device config
  writeFile(SPIFFS, global.deviceMetaFileName.c_str(), deviceConfig);
}
