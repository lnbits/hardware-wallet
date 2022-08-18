
CommandResponse executeDhExchange(String publicKeyHex) {
  if ((millis() - global.startTime) > 60 * 1000) {
    Serial.println(COMMAND_DH_EXCHANGE + " 1 " + " connection_period_expired");
    return {"Connection Refused", "60 secs from reboot only"};
  }

  // getAllButtonStates
  // if one down, allow
  // if (global.dhe_shared_secret != EMPTY_32) {
    // check one button pressed
  // }


  logSerial("### publicKeyHex: " + publicKeyHex);
  String tempMnemonic = generateMnemonic(24);

  byte dhe_secret[32];
  mnemonicToEntropy(tempMnemonic, dhe_secret, sizeof(dhe_secret));
  logSerial("### dhe_secret: " + toHex(dhe_secret, 32));

  PrivateKey dhPrivateKey(dhe_secret);
  PublicKey dhPublicKey = dhPrivateKey.publicKey();
  logSerial("### dhPublicKey: " + toHex(dhPublicKey.point, 64));

  byte publicKeyBin[64];
  fromHex(publicKeyHex, publicKeyBin, 64);
  PublicKey otherDhPublicKey(publicKeyBin, false);
  dhPrivateKey.ecdh(otherDhPublicKey, global.dhe_shared_secret, false);

  logSerial("### dhe_shared_secret: " + toHex(global.dhe_shared_secret, sizeof(global.dhe_shared_secret)));
  Serial.println(COMMAND_DH_EXCHANGE + " 0 " + toHex(dhPublicKey.point, sizeof(dhPublicKey.point)));
  logSerial("sent: " + COMMAND_DH_EXCHANGE + toHex(dhPublicKey.point, sizeof(dhPublicKey.point)));

  deleteFile(SPIFFS, global.sharedSecretFileName.c_str());
  writeFile(SPIFFS, global.sharedSecretFileName.c_str(), toHex(global.dhe_shared_secret, sizeof(global.dhe_shared_secret)));
  
  return {"Connected", "Encrypted connection"};
}
