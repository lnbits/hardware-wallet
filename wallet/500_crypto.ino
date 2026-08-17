String hashStringData(String key) {
  byte hash[SHA256_LEN] = { 0 };
  if (wally_sha256(
        (const uint8_t *)key.c_str(), key.length(), hash, sizeof(hash)
      ) != WALLY_OK) {
    clearSensitiveBytes(hash, sizeof(hash));
    return "";
  }
  String result = bytesToHexString(hash, sizeof(hash));
  clearSensitiveBytes(hash, sizeof(hash));
  return result;
}

String encryptData(byte key[32], byte iv[16], String msg) {
  // String has a trailing `null` character
  // String.getBytes() can overwrite the last character with `null`.
  // Add some extra-padding for safety
  String data = msg + "        ";

  // Pad data for encryption. Length must be multiple of 16.
  while (data.length() % 16 != 0) data += " ";

  int byteSize = data.length();
  byte messageBin[byteSize];
  data.getBytes(messageBin, byteSize);

  AES_ctx ctx;
  AES_init_ctx_iv(&ctx, key, iv);

  AES_CBC_encrypt_buffer(&ctx, messageBin, sizeof(messageBin));

  String result = bytesToHexString(messageBin, sizeof(messageBin));
  clearSensitiveBytes(messageBin, sizeof(messageBin));
  return result;
}

String encryptDataWithIv(byte key[32], String msg) {
  String data = String(msg.length()) + " " + msg;

  // create random initialization vector
  const int ivSize = 16;
  uint8_t iv[ivSize] = {0};
  if (!deriveHealthyHardwareEntropy(iv, ivSize)) {
    clearSensitiveBytes(iv, sizeof(iv));
    return "";
  }
  String ivHex = bytesToHexString(iv, ivSize);

  String messageHex = encryptData(key, iv, data);
  clearSensitiveBytes(iv, sizeof(iv));

  return messageHex + ivHex;
}


String decryptData(byte key[32], byte iv[16], String messageHex) {
  int byteSize =  messageHex.length() / 2;
  byte messageBin[byteSize + 1];
  if (!hexStringToBytes(messageHex, messageBin, byteSize)) {
    clearSensitiveBytes(messageBin, sizeof(messageBin));
    return "";
  }


  AES_ctx ctx;
  AES_init_ctx_iv(&ctx, key, iv);
  AES_CBC_decrypt_buffer(&ctx, messageBin, byteSize);
  messageBin[byteSize] = '\0';

  String result = String((char *)messageBin).substring(0, byteSize);
  clearSensitiveBytes(messageBin, sizeof(messageBin));
  return result;
}

String decryptDataWithIv(byte key[32], String messageWithIvHex) {
  int ivSize = 16;
  String messageHex = messageWithIvHex.substring(0, messageWithIvHex.length() - ivSize * 2);
  String ivHex = messageWithIvHex.substring(messageWithIvHex.length() - ivSize * 2, messageWithIvHex.length());

  uint8_t iv[ivSize];
  if (!hexStringToBytes(ivHex, iv, ivSize)) {
    clearSensitiveBytes(iv, sizeof(iv));
    return "";
  }
  String decryptedData = decryptData(key, iv, messageHex);
  clearSensitiveBytes(iv, sizeof(iv));

  Command c = extractCommand(decryptedData);
  int commandLength = c.cmd.toInt();
  return c.data.substring(0, commandLength);
}
