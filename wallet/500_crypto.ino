String hashPassword(String key) {
  byte hash[64] = { 0 };
  int hashLen = 0;
  hashLen = sha256(key, hash);
  return toHex(hash, hashLen);
}

String createMnemonic(int numberOfWords) {
  String mn = generateMnemonic(numberOfWords);
  return mn;
}


String encryptData(byte key[32], String ivHex, String msg) {
  int ivSize = 16;
  uint8_t iv[ivSize];
  fromHex(ivHex, iv, ivSize);

  byte messageBin[msg.length()];
  msg.getBytes(messageBin, msg.length());

  AES_ctx ctx;
  AES_init_ctx_iv(&ctx, key, iv);

  AES_CBC_encrypt_buffer(&ctx, messageBin, sizeof(messageBin));

  return toHex(messageBin, sizeof(messageBin));
}

String decryptData(byte key[32], String ivHex, String messageHex) {
  int byteSize =  messageHex.length() / 2;
  byte messageBin[byteSize];
  fromHex(messageHex, messageBin, byteSize);

  uint8_t iv[16];
  fromHex(ivHex, iv, 16);


  AES_ctx ctx;
  AES_init_ctx_iv(&ctx, key, iv);
  AES_CBC_decrypt_buffer(&ctx, messageBin, sizeof(messageBin));

  return String((char *)messageBin).substring(0, byteSize);
}
