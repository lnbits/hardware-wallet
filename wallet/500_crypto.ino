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
