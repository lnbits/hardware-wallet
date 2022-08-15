
CommandResponse executeDhExchange(String publicKeyHex) {
  logSerial("### publicKeyHex: " + publicKeyHex);
  String tempMnemonic = createMnemonic(24);

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
  Serial.println(COMMAND_DH_EXCHANGE + " " + toHex(dhPublicKey.point, sizeof(dhPublicKey.point)));
  logSerial("sent: " + COMMAND_DH_EXCHANGE + " " + toHex(dhPublicKey.point, sizeof(dhPublicKey.point)));
  return {"Connected", "Encrypted connection"};
}
