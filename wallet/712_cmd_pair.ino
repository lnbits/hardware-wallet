
CommandResponse executePair(String data) {
  if ((millis() - global.startTime) > 10 * 1000) {
    Serial.println(COMMAND_PAIR + " 1 " + " connection_period_expired");
    return {"Connection Refused", "10 secs from reboot only"};
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

  deleteFile(SPIFFS, global.sharedSecretFileName.c_str());
  writeFile(SPIFFS, global.sharedSecretFileName.c_str(), toHex(global.dhe_shared_secret, sizeof(global.dhe_shared_secret)));

  // update device config
  writeFile(SPIFFS, global.deviceMetaFileName.c_str(), global.deviceId + " " + button1Pin + " " + button2Pin);

  return {"Connected", "Encrypted connection"};
}

// DeviceConfig extractDeviceConfig(String data) {
//   int spacePos = data.indexOf(" ");
//   if (spacePos == -1 ) {
//     return {data, 0, 0};
//   }

//   String value = data.substring(0, spacePos);
//   data = data.substring(spacePos + 1, data.length() )
//   spacePos = data.indexOf(" ");
//   if (spacePos == -1 ) {
//     return {value, 0, 0};
//   }
//   String button1Pin = data.substring(0, spacePos);

//   data = data.substring(spacePos + 1, data.length() )
//   spacePos = data.indexOf(" ");
//   if (spacePos == -1 ) {
//     return {value, button1Pin.toInt(), 0};
//   }
//   String button2Pin = data.substring(spacePos + 1, data.length() );
//   return {value, button1Pin.toInt(), button2Pin.toInt()};
// }
