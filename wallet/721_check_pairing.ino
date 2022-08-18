CommandResponse executeCheckSecureConnection(String encryptedData) {
  String sharedSecret = toHex(global.dhe_shared_secret, sizeof(global.dhe_shared_secret));
  logSerial("sharedSecret 1:" + sharedSecret);
  logSerial("encryptedData 1:" + encryptedData);
  if (sharedSecret.equals("0000000000000000000000000000000000000000000000000000000000000000")) {
    logSerial("##### executeCheckSecureConnection empty");
    return {"Ready", "For encrypted connection"};
  }
  String data = decryptDataWithIv(global.dhe_shared_secret, encryptedData);
  logSerial("data 1:" + data);
  if (data == PING_VALUE) {
    Serial.println(COMMAND_CHECK_PAIRING + " 0 " + encryptDataWithIv(global.dhe_shared_secret, PING_VALUE));
    return {"Connected", "Encrypted connection"};
  }
  return {"Must re-pair", "Press & hold any button and reconnect"};
}