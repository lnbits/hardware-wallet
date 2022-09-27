
CommandResponse executePair(String data) {
  // if ((millis() - global.startTime) > 10 * 1000) {
  //   Serial.println(COMMAND_PAIR + " 1 " + " connection_period_expired");
  //   return {"Connection Refused", "10 secs from reboot passed"};
  // }

  String publicKeyHex = getWordAtPosition(data, 0);
  String button1Pin = getWordAtPosition(data, 1);
  String button2Pin = getWordAtPosition(data, 2);
  String persistSecrets = getWordAtPosition(data, 3);

  if (persistSecrets == "false") {
    global.persistSecrets = false;
  }

  if (isNotEmptyParam(publicKeyHex)) {
    return pair(publicKeyHex, button1Pin, button2Pin);
  }

  return {"Pairing", "Not encrypted"};
}

CommandResponse pair(String publicKeyHex, String button1Pin, String button2Pin) {
  String tempMnemonic = generateStrongerMnemonic(24);
  byte dhe_secret[32];
  mnemonicToEntropy(tempMnemonic, dhe_secret, sizeof(dhe_secret));

  PrivateKey dhPrivateKey(dhe_secret);
  PublicKey dhPublicKey = dhPrivateKey.publicKey();

  byte publicKeyBin[64];
  fromHex(publicKeyHex, publicKeyBin, 64);
  PublicKey otherDhPublicKey(publicKeyBin, false);
  dhPrivateKey.ecdh(otherDhPublicKey, global.dhe_shared_secret, false);

  Serial.println(COMMAND_PAIR + " 0 " + toHex(dhPublicKey.point, sizeof(dhPublicKey.point)));

  String sharedSecretHex =  toHex(global.dhe_shared_secret, sizeof(global.dhe_shared_secret));
  deleteFile(SPIFFS, global.sharedSecretFileName.c_str());
  writeFile(SPIFFS, global.sharedSecretFileName.c_str(), sharedSecretHex);

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