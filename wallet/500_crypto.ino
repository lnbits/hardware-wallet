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

String encryptData(byte key[32], byte iv[16], const String &msg) {
  // String has a trailing `null` character
  // String.getBytes() can overwrite the last character with `null`.
  // Add some extra-padding for safety
  String data = msg + "        ";

  // Pad data for encryption. Length must be multiple of 16.
  while (data.length() % 16 != 0) data += " ";

  const size_t byteSize = data.length();
  if (byteSize == 0 || byteSize % 16 != 0) return "";
  byte *messageBin = (byte *)malloc(byteSize);
  if (messageBin == NULL) return "";
  data.getBytes(messageBin, byteSize);

  AES_ctx ctx;
  AES_init_ctx_iv(&ctx, key, iv);

  AES_CBC_encrypt_buffer(&ctx, messageBin, byteSize);

  String result = bytesToHexString(messageBin, byteSize);
  clearSensitiveBytes(messageBin, byteSize);
  free(messageBin);
  return result;
}

String encryptDataWithIv(byte key[32], const String &msg) {
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


String decryptData(byte key[32], byte iv[16], const String &messageHex) {
  if (messageHex.length() == 0 || messageHex.length() % 32 != 0 ||
      !isStrictHex(messageHex)) return "";
  const size_t byteSize = messageHex.length() / 2;
  byte *messageBin = (byte *)malloc(byteSize + 1);
  if (messageBin == NULL) return "";
  if (!hexStringToBytes(messageHex, messageBin, byteSize)) {
    clearSensitiveBytes(messageBin, byteSize + 1);
    free(messageBin);
    return "";
  }


  AES_ctx ctx;
  AES_init_ctx_iv(&ctx, key, iv);
  AES_CBC_decrypt_buffer(&ctx, messageBin, byteSize);
  messageBin[byteSize] = '\0';

  String result = String((char *)messageBin).substring(0, byteSize);
  clearSensitiveBytes(messageBin, byteSize + 1);
  free(messageBin);
  return result;
}

String decryptDataWithIv(byte key[32], const String &messageWithIvHex) {
  const size_t ivSize = 16;
  if (messageWithIvHex.length() < ivSize * 2 + 32 ||
      (messageWithIvHex.length() - ivSize * 2) % 32 != 0 ||
      !isStrictHex(messageWithIvHex)) return "";
  String messageHex = messageWithIvHex.substring(0, messageWithIvHex.length() - ivSize * 2);
  String ivHex = messageWithIvHex.substring(messageWithIvHex.length() - ivSize * 2, messageWithIvHex.length());

  uint8_t iv[ivSize];
  if (!hexStringToBytes(ivHex, iv, ivSize)) {
    clearSensitiveBytes(iv, sizeof(iv));
    return "";
  }
  String decryptedData = decryptData(key, iv, messageHex);
  clearSensitiveBytes(iv, sizeof(iv));

  const int separator = decryptedData.indexOf(' ');
  if (separator <= 0) return "";
  size_t commandLength = 0;
  for (int i = 0; i < separator; i++) {
    const char c = decryptedData[i];
    if (c < '0' || c > '9') return "";
    const size_t digit = (size_t)(c - '0');
    if (commandLength > (SIZE_MAX - digit) / 10) return "";
    commandLength = commandLength * 10 + digit;
  }
  const size_t payloadStart = (size_t)separator + 1;
  if (commandLength > decryptedData.length() - payloadStart) return "";
  return decryptedData.substring(payloadStart, payloadStart + commandLength);
}
