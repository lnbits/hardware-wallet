
CommandResponse executePair(String data) {
  if ((millis() - global.startTime) > 10 * 1000) {
    Serial.println(COMMAND_PAIR + " 1 " + " connection_period_expired");
    return {"Connection Refused", "10 secs from reboot passed"};
  }

  String publicKeyHex = getWordAtPosition(data, 0);
  String button1Pin = getWordAtPosition(data, 1);
  String button2Pin = getWordAtPosition(data, 2);

  logSerial("pairing: " + publicKeyHex + " btn1:" + String(button1Pin.toInt()) + " btn2:" + String(button2Pin.toInt()));


  String tempMnemonic = generateMnemonic(24);

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

  // update device config
  writeFile(SPIFFS, global.deviceMetaFileName.c_str(), global.deviceId + " " + button1Pin + " " + button2Pin);

  String fingerprint = hashStringData(sharedSecretHex).substring(0, 5);
  fingerprint.toUpperCase();
  return {"Confirm", "Code: " + fingerprint };
}
