// Password-protected wallet storage. Version 2 never stores the key used to
// encrypt the mnemonic. The key is derived only after the user supplies the
// password, and the encrypted record is authenticated before decryption.

const String WALLET_STORAGE_VERSION = "v2";
const uint32_t WALLET_KDF_ITERATIONS = 100000;
const size_t WALLET_SALT_SIZE = 16;
const size_t WALLET_KEY_SIZE = 32;

String walletRecordPayload(
  uint32_t iterations,
  const String &saltHex,
  const String &verifierHex,
  const String &encryptedMnemonic
) {
  return WALLET_STORAGE_VERSION + ":" + String(iterations) + ":" +
         saltHex + ":" + verifierHex + ":" + encryptedMnemonic;
}

void deriveWalletKeys(
  const String &password,
  const uint8_t *salt,
  uint32_t iterations,
  uint8_t *encryptionKey,
  uint8_t *authenticationKey
) {
  uint8_t derived[WALLET_KEY_SIZE * 2] = {0};
  pbkdf2_hmac_sha256(
    (const uint8_t *)password.c_str(),
    password.length(),
    salt,
    WALLET_SALT_SIZE,
    iterations,
    derived,
    sizeof(derived)
  );
  memcpy(encryptionKey, derived, WALLET_KEY_SIZE);
  memcpy(authenticationKey, derived + WALLET_KEY_SIZE, WALLET_KEY_SIZE);
  clearSensitiveBytes(derived, sizeof(derived));
}

String walletHmacHex(const uint8_t *key, const String &data) {
  uint8_t mac[32] = {0};
  sha256Hmac(
    key,
    WALLET_KEY_SIZE,
    (const uint8_t *)data.c_str(),
    data.length(),
    mac
  );
  String result = toHex(mac, sizeof(mac));
  clearSensitiveBytes(mac, sizeof(mac));
  return result;
}

String walletPasswordVerifier(const uint8_t *authenticationKey) {
  return walletHmacHex(
    authenticationKey,
    "Bowser HWW password verifier v2"
  );
}

bool constantTimeStringEquals(const String &left, const String &right) {
  size_t leftLength = left.length();
  size_t rightLength = right.length();
  size_t length = leftLength > rightLength ? leftLength : rightLength;
  uint8_t difference = (uint8_t)(leftLength ^ rightLength);
  for (size_t i = 0; i < length; i++) {
    uint8_t leftByte = i < leftLength ? left[i] : 0;
    uint8_t rightByte = i < rightLength ? right[i] : 0;
    difference |= leftByte ^ rightByte;
  }
  return difference == 0;
}

bool generateWalletSalt(uint8_t *salt) {
  bootloader_random_enable();
  bool healthy = hardwareRngPassesHealthCheck();
  if (healthy) esp_fill_random(salt, WALLET_SALT_SIZE);
  bootloader_random_disable();
  return healthy;
}

bool parseProtectedWalletRecord(const String &record) {
  size_t separatorCount = 0;
  for (size_t i = 0; i < record.length(); i++) {
    if (record[i] == ':') separatorCount++;
  }

  String version = getTokenAtPosition(record, ":", 0);
  String iterationsString = getTokenAtPosition(record, ":", 1);
  String saltHex = getTokenAtPosition(record, ":", 2);
  String verifierHex = getTokenAtPosition(record, ":", 3);
  String encryptedMnemonic = getTokenAtPosition(record, ":", 4);
  String mnemonicMac = getTokenAtPosition(record, ":", 5);
  uint32_t iterations = (uint32_t)iterationsString.toInt();

  if (
    record.length() > 1200 ||
    separatorCount != 5 ||
    version != WALLET_STORAGE_VERSION ||
    iterations != WALLET_KDF_ITERATIONS ||
    saltHex.length() != WALLET_SALT_SIZE * 2 ||
    verifierHex.length() != 64 ||
    encryptedMnemonic.length() < 64 || encryptedMnemonic.length() > 1024 ||
    encryptedMnemonic.length() % 32 != 0 ||
    mnemonicMac.length() != 64 ||
    !isStrictHex(saltHex) ||
    !isStrictHex(verifierHex) ||
    !isStrictHex(encryptedMnemonic) ||
    !isStrictHex(mnemonicMac)
  ) return false;

  global.passwordSalt = saltHex;
  global.passwordVerifier = verifierHex;
  global.encryptedMnemonic = encryptedMnemonic;
  global.mnemonicMac = mnemonicMac;
  global.passwordKdfIterations = iterations;
  global.legacyWalletStorage = false;
  global.mnemonic = "";
  return true;
}

bool writeProtectedWalletRecord(const String &record) {
  String temporaryPath = global.walletFileName + ".tmp";
  String backupPath = global.walletFileName + ".bak";
  SPIFFS.remove(temporaryPath.c_str());

  File temporary = SPIFFS.open(temporaryPath.c_str(), FILE_WRITE);
  if (!temporary) return false;
  size_t written = temporary.print(record);
  temporary.close();
  if (written != record.length()) {
    SPIFFS.remove(temporaryPath.c_str());
    return false;
  }

  FileData verification = readFile(SPIFFS, temporaryPath.c_str());
  if (!verification.success || verification.data != record) {
    SPIFFS.remove(temporaryPath.c_str());
    return false;
  }

  SPIFFS.remove(backupPath.c_str());
  if (
    SPIFFS.exists(global.walletFileName.c_str()) &&
    !SPIFFS.rename(global.walletFileName.c_str(), backupPath.c_str())
  ) {
    SPIFFS.remove(temporaryPath.c_str());
    return false;
  }
  if (!SPIFFS.rename(temporaryPath.c_str(), global.walletFileName.c_str())) {
    if (SPIFFS.exists(backupPath.c_str())) {
      SPIFFS.rename(backupPath.c_str(), global.walletFileName.c_str());
    }
    return false;
  }

  FileData committed = readFile(SPIFFS, global.walletFileName.c_str());
  if (!committed.success || committed.data != record) {
    SPIFFS.remove(global.walletFileName.c_str());
    if (SPIFFS.exists(backupPath.c_str())) {
      SPIFFS.rename(backupPath.c_str(), global.walletFileName.c_str());
    }
    return false;
  }

  SPIFFS.remove(backupPath.c_str());
  deleteFile(SPIFFS, global.passwordFileName.c_str());
  deleteFile(SPIFFS, global.mnemonicFileName.c_str());
  return true;
}

bool initializeProtectedWallet(
  const String &password,
  const String &mnemonic,
  bool persistSecrets
) {
  uint8_t salt[WALLET_SALT_SIZE] = {0};
  uint8_t encryptionKey[WALLET_KEY_SIZE] = {0};
  uint8_t authenticationKey[WALLET_KEY_SIZE] = {0};

  if (!generateWalletSalt(salt)) {
    clearSensitiveBytes(salt, sizeof(salt));
    return false;
  }
  deriveWalletKeys(
    password,
    salt,
    WALLET_KDF_ITERATIONS,
    encryptionKey,
    authenticationKey
  );

  String saltHex = toHex(salt, sizeof(salt));
  String verifierHex = walletPasswordVerifier(authenticationKey);
  String encryptedMnemonic = encryptDataWithIv(encryptionKey, mnemonic);
  clearSensitiveBytes(salt, sizeof(salt));
  clearSensitiveBytes(encryptionKey, sizeof(encryptionKey));
  if (encryptedMnemonic == "") {
    clearSensitiveBytes(authenticationKey, sizeof(authenticationKey));
    return false;
  }

  String payload = walletRecordPayload(
    WALLET_KDF_ITERATIONS,
    saltHex,
    verifierHex,
    encryptedMnemonic
  );
  String mnemonicMac = walletHmacHex(authenticationKey, payload);
  clearSensitiveBytes(authenticationKey, sizeof(authenticationKey));
  String record = payload + ":" + mnemonicMac;

  if (persistSecrets && !writeProtectedWalletRecord(record)) return false;

  global.passwordSalt = saltHex;
  global.passwordVerifier = verifierHex;
  global.encryptedMnemonic = encryptedMnemonic;
  global.mnemonicMac = mnemonicMac;
  global.passwordKdfIterations = WALLET_KDF_ITERATIONS;
  global.legacyWalletStorage = false;
  return true;
}

bool unlockProtectedWallet(const String &password) {
  if (global.legacyWalletStorage) {
    String legacyPasswordHash = hashStringData(password);
    if (!constantTimeStringEquals(legacyPasswordHash, global.passwordVerifier)) {
      return false;
    }

    uint8_t legacyKey[WALLET_KEY_SIZE] = {0};
    if (
      !isStrictHex(legacyPasswordHash) ||
      fromHex(legacyPasswordHash, legacyKey, sizeof(legacyKey)) != sizeof(legacyKey)
    ) {
      clearSensitiveBytes(legacyKey, sizeof(legacyKey));
      return false;
    }
    String mnemonic = decryptDataWithIv(legacyKey, global.encryptedMnemonic);
    clearSensitiveBytes(legacyKey, sizeof(legacyKey));
    if (!checkMnemonic(mnemonic)) {
      mnemonic = "";
      return false;
    }

    // The correct password is the only point at which a legacy wallet can be
    // migrated, because old firmware stored no independent encryption salt.
    if (!initializeProtectedWallet(password, mnemonic, true)) {
      mnemonic = "";
      return false;
    }
    global.mnemonic = mnemonic;
    return true;
  }

  uint8_t salt[WALLET_SALT_SIZE] = {0};
  uint8_t encryptionKey[WALLET_KEY_SIZE] = {0};
  uint8_t authenticationKey[WALLET_KEY_SIZE] = {0};
  if (
    global.passwordSalt.length() != WALLET_SALT_SIZE * 2 ||
    fromHex(global.passwordSalt, salt, sizeof(salt)) != sizeof(salt)
  ) return false;

  deriveWalletKeys(
    password,
    salt,
    global.passwordKdfIterations,
    encryptionKey,
    authenticationKey
  );
  clearSensitiveBytes(salt, sizeof(salt));

  String verifierHex = walletPasswordVerifier(authenticationKey);
  String payload = walletRecordPayload(
    global.passwordKdfIterations,
    global.passwordSalt,
    global.passwordVerifier,
    global.encryptedMnemonic
  );
  String expectedMac = walletHmacHex(authenticationKey, payload);
  bool verified =
    constantTimeStringEquals(verifierHex, global.passwordVerifier) &&
    constantTimeStringEquals(expectedMac, global.mnemonicMac);
  clearSensitiveBytes(authenticationKey, sizeof(authenticationKey));
  if (!verified) {
    clearSensitiveBytes(encryptionKey, sizeof(encryptionKey));
    return false;
  }

  String mnemonic = decryptDataWithIv(encryptionKey, global.encryptedMnemonic);
  clearSensitiveBytes(encryptionKey, sizeof(encryptionKey));
  if (!checkMnemonic(mnemonic)) {
    mnemonic = "";
    return false;
  }
  global.mnemonic = mnemonic;
  return true;
}

bool loadProtectedWalletFile(const String &path) {
  FileData file = readFile(SPIFFS, path.c_str());
  return file.success && parseProtectedWalletRecord(file.data);
}

bool loadWalletStorage() {
  String temporaryPath = global.walletFileName + ".tmp";
  String backupPath = global.walletFileName + ".bak";

  // Prefer a complete temporary record left by an interrupted atomic update.
  if (loadProtectedWalletFile(temporaryPath)) {
    SPIFFS.remove(global.walletFileName.c_str());
    SPIFFS.rename(temporaryPath.c_str(), global.walletFileName.c_str());
    SPIFFS.remove(backupPath.c_str());
    return true;
  }
  SPIFFS.remove(temporaryPath.c_str());

  if (loadProtectedWalletFile(global.walletFileName)) {
    SPIFFS.remove(backupPath.c_str());
    return true;
  }
  if (loadProtectedWalletFile(backupPath)) {
    SPIFFS.remove(global.walletFileName.c_str());
    SPIFFS.rename(backupPath.c_str(), global.walletFileName.c_str());
    return true;
  }

  // Legacy storage is retained until a successful password login migrates it.
  FileData passwordFile = readFile(SPIFFS, global.passwordFileName.c_str());
  FileData mnemonicFile = readFile(SPIFFS, global.mnemonicFileName.c_str());
  if (
    !passwordFile.success || !mnemonicFile.success ||
    passwordFile.data.length() != 64 ||
    !isStrictHex(passwordFile.data) ||
    mnemonicFile.data.length() < 64 ||
    mnemonicFile.data.length() % 32 != 0 ||
    !isStrictHex(mnemonicFile.data)
  ) return false;

  global.passwordVerifier = passwordFile.data;
  global.encryptedMnemonic = mnemonicFile.data;
  global.passwordSalt = "";
  global.mnemonicMac = "";
  global.passwordKdfIterations = 0;
  global.legacyWalletStorage = true;
  global.mnemonic = "";
  return true;
}
