// Thin Bowser policy helpers around libwally's public C API. These functions
// deliberately expose libwally's native ext_key and wally_psbt types rather
// than maintaining a compatibility object model.

size_t getMnemonicBytesForWordCount(int words) {
  switch (words) {
    case 12: return 16;
    case 15: return 20;
    case 18: return 24;
    case 21: return 28;
    case 24: return 32;
    default: return 0;
  }
}

String bytesToHexString(const uint8_t *bytes, size_t length) {
  char *hex = NULL;
  if (wally_hex_from_bytes(bytes, length, &hex) != WALLY_OK || hex == NULL) return "";
  String result(hex);
  wally_free_string(hex);
  return result;
}

bool hexStringToBytes(const String &hex, uint8_t *output, size_t outputLength) {
  size_t written = 0;
  if (hex.length() != outputLength * 2) return false;
  return wally_hex_n_to_bytes(
    hex.c_str(), hex.length(), output, outputLength, &written
  ) == WALLY_OK && written == outputLength;
}

String mnemonicFromBytes(const uint8_t *entropy, size_t entropyLength) {
  char *mnemonic = NULL;
  if (bip39_mnemonic_from_bytes(NULL, entropy, entropyLength, &mnemonic) != WALLY_OK ||
      mnemonic == NULL) return "";
  String result(mnemonic);
  wally_free_string(mnemonic);
  return result;
}

bool mnemonicIsValid(const String &mnemonic) {
  return bip39_mnemonic_validate(NULL, mnemonic.c_str()) == WALLY_OK;
}

bool mnemonicToBytes(const String &mnemonic, uint8_t *output, size_t outputLength) {
  size_t written = 0;
  return bip39_mnemonic_to_bytes(
    NULL, mnemonic.c_str(), output, outputLength, &written
  ) == WALLY_OK && written == outputLength;
}

bool isMainnetName(const String &networkName, bool *mainnet) {
  if (networkName == "Mainnet") {
    *mainnet = true;
    return true;
  }
  if (networkName == "Testnet") {
    *mainnet = false;
    return true;
  }
  return false;
}

bool deriveRootKey(bool mainnet, struct ext_key *root) {
  uint8_t seed[BIP39_SEED_LEN_512] = {0};
  const char *passphrase = global.passphrase.length() ? global.passphrase.c_str() : NULL;
  if (bip39_mnemonic_to_seed512(
        global.mnemonic.c_str(), passphrase, seed, sizeof(seed)
      ) != WALLY_OK) {
    clearSensitiveBytes(seed, sizeof(seed));
    return false;
  }
  const uint32_t version = mainnet ? BIP32_VER_MAIN_PRIVATE : BIP32_VER_TEST_PRIVATE;
  const bool success = bip32_key_from_seed(seed, sizeof(seed), version, 0, root) == WALLY_OK;
  clearSensitiveBytes(seed, sizeof(seed));
  return success;
}

bool derivePathKey(const struct ext_key *root, const String &path, struct ext_key *derived) {
  if (path.length() == 0 || path.length() > 255) return false;
  return bip32_key_from_parent_path_str(
    root, path.c_str(), 0, BIP32_FLAG_KEY_PRIVATE | BIP32_FLAG_ALLOW_UPPER, derived
  ) == WALLY_OK;
}

bool parseWalletPath(const String &path, uint32_t *components,
                     size_t capacity, size_t *componentCount) {
  *componentCount = 0;
  return bip32_path_from_str(
    path.c_str(), 0, 0, BIP32_FLAG_ALLOW_UPPER,
    components, capacity, componentCount
  ) == WALLY_OK && *componentCount > 0 && *componentCount <= capacity;
}

bool standardPathMatchesNetwork(const String &path, bool mainnet) {
  uint32_t components[8] = {0};
  size_t count = 0;
  if (!parseWalletPath(path, components, 8, &count) || count < 2) return false;
  const bool standardPurpose =
    components[0] == BIP32_INITIAL_HARDENED_CHILD + 44 ||
    components[0] == BIP32_INITIAL_HARDENED_CHILD + 48 ||
    components[0] == BIP32_INITIAL_HARDENED_CHILD + 49 ||
    components[0] == BIP32_INITIAL_HARDENED_CHILD + 84 ||
    components[0] == BIP32_INITIAL_HARDENED_CHILD + 86;
  return standardPurpose &&
         components[1] == BIP32_INITIAL_HARDENED_CHILD + (mainnet ? 0 : 1);
}

uint32_t slip132PublicVersion(bool mainnet, const String &path) {
  uint32_t components[8] = {0};
  size_t count = 0;
  if (!parseWalletPath(path, components, 8, &count)) {
    return mainnet ? BIP32_VER_MAIN_PUBLIC : BIP32_VER_TEST_PUBLIC;
  }
  if (components[0] == BIP32_INITIAL_HARDENED_CHILD + 49) {
    return mainnet ? 0x049d7cb2 : 0x044a5262;
  }
  if (components[0] == BIP32_INITIAL_HARDENED_CHILD + 84) {
    return mainnet ? 0x04b24746 : 0x045f1cf6;
  }
  if (components[0] == BIP32_INITIAL_HARDENED_CHILD + 48 && count >= 4) {
    if (components[3] == BIP32_INITIAL_HARDENED_CHILD + 1) {
      return mainnet ? 0x0295b43f : 0x024289ef;
    }
    if (components[3] == BIP32_INITIAL_HARDENED_CHILD + 2) {
      return mainnet ? 0x02aa7ed3 : 0x02575483;
    }
  }
  return mainnet ? BIP32_VER_MAIN_PUBLIC : BIP32_VER_TEST_PUBLIC;
}

String serializeAccountPublicKey(
  const struct ext_key *account,
  bool mainnet,
  const String &path
) {
  uint8_t serialized[BIP32_SERIALIZED_LEN] = {0};
  if (bip32_key_serialize(
        account, BIP32_FLAG_KEY_PUBLIC, serialized, sizeof(serialized)
      ) != WALLY_OK) return "";

  const uint32_t version = slip132PublicVersion(mainnet, path);
  serialized[0] = (uint8_t)(version >> 24);
  serialized[1] = (uint8_t)(version >> 16);
  serialized[2] = (uint8_t)(version >> 8);
  serialized[3] = (uint8_t)version;
  char *encoded = NULL;
  if (wally_base58_from_bytes(
        serialized, sizeof(serialized), BASE58_FLAG_CHECKSUM, &encoded
      ) != WALLY_OK || encoded == NULL) {
    clearSensitiveBytes(serialized, sizeof(serialized));
    return "";
  }
  String result(encoded);
  wally_free_string(encoded);
  clearSensitiveBytes(serialized, sizeof(serialized));
  return result;
}

String rootFingerprint(const struct ext_key *root) {
  return bytesToHexString(root->hash160, BIP32_KEY_FINGERPRINT_LEN);
}

bool p2trKeySpendScriptFromPublicKey(
  const uint8_t *publicKey,
  size_t publicKeyLength,
  uint8_t *script,
  size_t scriptCapacity,
  size_t *scriptLength
) {
  uint8_t tweaked[EC_PUBLIC_KEY_LEN] = {0};
  const bool success =
    wally_ec_public_key_bip341_tweak(
      publicKey, publicKeyLength, NULL, 0, 0,
      tweaked, sizeof(tweaked)
    ) == WALLY_OK &&
    wally_witness_program_from_bytes_and_version(
      tweaked + 1, EC_XONLY_PUBLIC_KEY_LEN, 1, 0,
      script, scriptCapacity, scriptLength
    ) == WALLY_OK;
  clearSensitiveBytes(tweaked, sizeof(tweaked));
  return success;
}

bool scriptForSingleKeyPath(
  const uint8_t *publicKey,
  const String &path,
  uint8_t *script,
  size_t scriptCapacity,
  size_t *scriptLength
) {
  uint32_t components[8] = {0};
  size_t componentCount = 0;
  if (!parseWalletPath(path, components, 8, &componentCount)) return false;
  const uint32_t purpose = components[0];
  if (purpose == BIP32_INITIAL_HARDENED_CHILD + 44) {
    return wally_scriptpubkey_p2pkh_from_bytes(
      publicKey, EC_PUBLIC_KEY_LEN, WALLY_SCRIPT_HASH160,
      script, scriptCapacity, scriptLength
    ) == WALLY_OK;
  }

  if (purpose == BIP32_INITIAL_HARDENED_CHILD + 86) {
    return p2trKeySpendScriptFromPublicKey(
      publicKey, EC_PUBLIC_KEY_LEN, script, scriptCapacity, scriptLength
    );
  }

  uint8_t witness[WALLY_SEGWIT_V0_ADDRESS_PUBKEY_MAX_LEN] = {0};
  size_t witnessLength = 0;
  if (wally_witness_program_from_bytes(
        publicKey, EC_PUBLIC_KEY_LEN, WALLY_SCRIPT_HASH160,
        witness, sizeof(witness), &witnessLength
      ) != WALLY_OK) return false;
  if (purpose == BIP32_INITIAL_HARDENED_CHILD + 49) {
    return wally_scriptpubkey_p2sh_from_bytes(
      witness, witnessLength, WALLY_SCRIPT_HASH160,
      script, scriptCapacity, scriptLength
    ) == WALLY_OK;
  }
  if (purpose != BIP32_INITIAL_HARDENED_CHILD + 84) return false;
  if (scriptCapacity < witnessLength) return false;
  memcpy(script, witness, witnessLength);
  *scriptLength = witnessLength;
  return true;
}

String addressFromScript(const uint8_t *script, size_t scriptLength, bool mainnet) {
  size_t scriptType = WALLY_SCRIPT_TYPE_UNKNOWN;
  if (wally_scriptpubkey_get_type(script, scriptLength, &scriptType) != WALLY_OK) return "";
  char *address = NULL;
  int result;
  if (scriptType == WALLY_SCRIPT_TYPE_P2WPKH ||
      scriptType == WALLY_SCRIPT_TYPE_P2WSH ||
      scriptType == WALLY_SCRIPT_TYPE_P2TR) {
    result = wally_addr_segwit_from_bytes(
      script, scriptLength, mainnet ? "bc" : "tb", 0, &address
    );
  } else if (scriptType == WALLY_SCRIPT_TYPE_P2PKH ||
             scriptType == WALLY_SCRIPT_TYPE_P2SH) {
    result = wally_scriptpubkey_to_address(
      script, scriptLength,
      mainnet ? WALLY_NETWORK_BITCOIN_MAINNET : WALLY_NETWORK_BITCOIN_TESTNET,
      &address
    );
  } else {
    return "";
  }
  if (result != WALLY_OK || address == NULL) return "";
  String value(address);
  wally_free_string(address);
  return value;
}

String addressForDerivedKey(const struct ext_key *key, const String &path, bool mainnet) {
  uint8_t script[WALLY_ADDRESS_PUBKEY_MAX_LEN] = {0};
  size_t scriptLength = 0;
  if (!scriptForSingleKeyPath(
        key->pub_key, path, script, sizeof(script), &scriptLength
      )) return "";
  return addressFromScript(script, scriptLength, mainnet);
}

int rawXCoordinate(
  unsigned char *output,
  const unsigned char *x32,
  const unsigned char *y32,
  void *data
) {
  (void)y32;
  (void)data;
  memcpy(output, x32, 32);
  return 1;
}

bool legacyCompatibleEcdh(
  const uint8_t privateKey[EC_PRIVATE_KEY_LEN],
  const uint8_t publicKey[EC_PUBLIC_KEY_UNCOMPRESSED_LEN],
  uint8_t output[SHA256_LEN]
) {
  secp256k1_pubkey parsed;
  secp256k1_context *context = wally_get_secp_context();
  if (context == NULL ||
      !secp256k1_ec_pubkey_parse(context, &parsed, publicKey, EC_PUBLIC_KEY_UNCOMPRESSED_LEN)) {
    clearSensitiveBytes((uint8_t *)&parsed, sizeof(parsed));
    return false;
  }
  const bool success = secp256k1_ecdh(
    context, output, &parsed, privateKey, rawXCoordinate, NULL
  ) == 1;
  clearSensitiveBytes((uint8_t *)&parsed, sizeof(parsed));
  return success;
}
