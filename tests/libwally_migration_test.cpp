#include <cassert>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

extern "C" {
#include "libwally.h"
}

namespace {

std::string readExamplePsbt() {
  std::ifstream input("examples/sd-card/ex01/commands.in.txt");
  std::stringstream contents;
  contents << input.rdbuf();
  const std::string marker = "/psbt Testnet ";
  const std::string text = contents.str();
  const size_t begin = text.find(marker);
  assert(begin != std::string::npos);
  const size_t valueBegin = begin + marker.size();
  const size_t end = text.find('\n', valueBegin);
  return text.substr(valueBegin, end - valueBegin);
}

void bip39AndBip32Work() {
  const char *mnemonic =
    "abandon abandon abandon abandon abandon abandon abandon abandon "
    "abandon abandon abandon about";
  const char *expectedSeed =
    "c55257c360c07c72029aebc1b53c05ed0362ada38ead3e3e9efa3708e5349553"
    "1f09a6987599d18264c1e1c92f2cf141630c7a3c4ab7c81b2f001698e7463b04";
  unsigned char seed[BIP39_SEED_LEN_512] = {0};
  unsigned char expected[BIP39_SEED_LEN_512] = {0};
  size_t written = 0;
  assert(bip39_mnemonic_validate(NULL, mnemonic) == WALLY_OK);
  assert(bip39_mnemonic_to_seed512(mnemonic, "TREZOR", seed, sizeof(seed)) == WALLY_OK);
  assert(wally_hex_to_bytes(expectedSeed, expected, sizeof(expected), &written) == WALLY_OK);
  assert(written == sizeof(expected));
  assert(std::memcmp(seed, expected, sizeof(seed)) == 0);

  struct ext_key root = {};
  struct ext_key account = {};
  assert(bip32_key_from_seed(seed, sizeof(seed), BIP32_VER_MAIN_PRIVATE, 0, &root) == WALLY_OK);
  assert(bip32_key_from_parent_path_str(
    &root, "m/84'/0'/0'", 0, BIP32_FLAG_KEY_PRIVATE, &account
  ) == WALLY_OK);
  unsigned char serialized[BIP32_SERIALIZED_LEN] = {0};
  assert(bip32_key_serialize(
    &account, BIP32_FLAG_KEY_PUBLIC, serialized, sizeof(serialized)
  ) == WALLY_OK);
  assert(serialized[0] == 0x04 && serialized[1] == 0x88 &&
         serialized[2] == 0xb2 && serialized[3] == 0x1e);
  wally_bzero(&account, sizeof(account));
  wally_bzero(&root, sizeof(root));
  wally_bzero(seed, sizeof(seed));
  wally_bzero(expected, sizeof(expected));
}

void walletStorageKdfIsCompatible() {
  const char *expectedHex =
    "0394a2ede332c9a13eb82e9b24631604c31df978b4e2f0fbd2c549944f9d79a"
    "536ceea9b92c6170cbbf0153ef33a4ff57321e17b7a5fadc33f7023ddd325da47";
  unsigned char derived[64] = {0};
  unsigned char expected[64] = {0};
  size_t written = 0;
  assert(wally_pbkdf2_hmac_sha256(
    reinterpret_cast<const unsigned char *>("password"), 8,
    reinterpret_cast<const unsigned char *>("salt"), 4,
    0, 100000, derived, sizeof(derived)
  ) == WALLY_OK);
  assert(wally_hex_to_bytes(expectedHex, expected, sizeof(expected), &written) == WALLY_OK);
  assert(written == sizeof(expected));
  assert(std::memcmp(derived, expected, sizeof(derived)) == 0);
  wally_bzero(derived, sizeof(derived));
  wally_bzero(expected, sizeof(expected));
}

void existingTestnetXpubIsCompatible() {
  const char *mnemonic =
    "link pool sudden unfair illness west sister helmet hard rally boring tool "
    "avoid fantasy solar company favorite net cluster truly miss reduce margin memory";
  const char *expected =
    "vpub5YXnnXQcawUCY4Pr9jZrm15sfAGAP2PnW8n2PskF48JSBwkvrjiGYiHrSvGk"
    "cBJg5gkhVkoiVf8DBTaigiiURi5Kt6hKEFYUmY3Kr5MWRCk";
  unsigned char seed[BIP39_SEED_LEN_512] = {0};
  struct ext_key root = {};
  struct ext_key account = {};
  unsigned char serialized[BIP32_SERIALIZED_LEN] = {0};
  assert(bip39_mnemonic_to_seed512(mnemonic, NULL, seed, sizeof(seed)) == WALLY_OK);
  assert(bip32_key_from_seed(seed, sizeof(seed), BIP32_VER_TEST_PRIVATE, 0, &root) == WALLY_OK);
  assert(bip32_key_from_parent_path_str(
    &root, "m/84'/1'/0'", 0, BIP32_FLAG_KEY_PRIVATE, &account
  ) == WALLY_OK);
  assert(bip32_key_serialize(
    &account, BIP32_FLAG_KEY_PUBLIC, serialized, sizeof(serialized)
  ) == WALLY_OK);
  const uint32_t vpub = 0x045f1cf6;
  serialized[0] = static_cast<unsigned char>(vpub >> 24);
  serialized[1] = static_cast<unsigned char>(vpub >> 16);
  serialized[2] = static_cast<unsigned char>(vpub >> 8);
  serialized[3] = static_cast<unsigned char>(vpub);
  char *encoded = NULL;
  assert(wally_base58_from_bytes(
    serialized, sizeof(serialized), BASE58_FLAG_CHECKSUM, &encoded
  ) == WALLY_OK);
  assert(std::strcmp(encoded, expected) == 0);
  wally_free_string(encoded);
  wally_bzero(&account, sizeof(account));
  wally_bzero(&root, sizeof(root));
  wally_bzero(seed, sizeof(seed));
}

void psbtSigningPreservesMetadata() {
  const std::string encoded = readExamplePsbt();
  struct wally_psbt *psbt = NULL;
  assert(wally_psbt_from_base64_n(
    encoded.c_str(), encoded.size(), WALLY_PSBT_PARSE_FLAG_STRICT, &psbt
  ) == WALLY_OK);
  assert(psbt != NULL && psbt->version == WALLY_PSBT_VERSION_0);
  assert(psbt->num_inputs == 2 && psbt->num_outputs == 2);
  for (size_t i = 0; i < psbt->num_inputs; ++i) {
    size_t signingLength = 0;
    assert(wally_psbt_get_input_signing_script_len(psbt, i, &signingLength) == WALLY_OK);
    std::vector<unsigned char> signingScript(signingLength);
    assert(wally_psbt_get_input_signing_script(
      psbt, i, signingScript.data(), signingScript.size(), &signingLength
    ) == WALLY_OK);
    size_t scriptType = WALLY_SCRIPT_TYPE_UNKNOWN;
    assert(wally_scriptpubkey_get_type(
      signingScript.data(), signingLength, &scriptType
    ) == WALLY_OK);
    assert(scriptType == WALLY_SCRIPT_TYPE_P2WPKH);
  }

  const unsigned char unknownKey[] = {0xfc, 'b', 'o', 'w', 's', 'e', 'r'};
  const unsigned char unknownValue[] = {0x01, 0x02, 0x03, 0x04};
  assert(wally_map_add(&psbt->unknowns, unknownKey, sizeof(unknownKey),
                       unknownValue, sizeof(unknownValue)) == WALLY_OK);
  assert(wally_map_add(&psbt->inputs[0].unknowns, unknownKey, sizeof(unknownKey),
                       unknownValue, sizeof(unknownValue)) == WALLY_OK);
  assert(wally_map_add(&psbt->outputs[0].unknowns, unknownKey, sizeof(unknownKey),
                       unknownValue, sizeof(unknownValue)) == WALLY_OK);

  const char *mnemonic =
    "link pool sudden unfair illness west sister helmet hard rally boring tool "
    "avoid fantasy solar company favorite net cluster truly miss reduce margin memory";
  unsigned char seed[BIP39_SEED_LEN_512] = {0};
  struct ext_key root = {};
  assert(bip39_mnemonic_to_seed512(mnemonic, NULL, seed, sizeof(seed)) == WALLY_OK);
  assert(bip32_key_from_seed(seed, sizeof(seed), BIP32_VER_TEST_PRIVATE, 0, &root) == WALLY_OK);
  size_t before[2] = {0};
  assert(wally_psbt_get_input_signatures_size(psbt, 0, &before[0]) == WALLY_OK);
  assert(wally_psbt_get_input_signatures_size(psbt, 1, &before[1]) == WALLY_OK);
  assert(wally_psbt_signing_cache_enable(psbt, 0) == WALLY_OK);
  assert(wally_psbt_sign_bip32(psbt, &root, EC_FLAG_GRIND_R) == WALLY_OK);
  assert(wally_psbt_signing_cache_disable(psbt) == WALLY_OK);
  for (size_t i = 0; i < 2; ++i) {
    size_t after = 0;
    assert(wally_psbt_get_input_signatures_size(psbt, i, &after) == WALLY_OK);
    assert(after > before[i]);
  }

  char *signedBase64 = NULL;
  assert(wally_psbt_to_base64(psbt, 0, &signedBase64) == WALLY_OK);
  assert(signedBase64 != NULL);
  struct wally_psbt *roundTripped = NULL;
  assert(wally_psbt_from_base64(
    signedBase64, WALLY_PSBT_PARSE_FLAG_STRICT, &roundTripped
  ) == WALLY_OK);
  assert(roundTripped->unknowns.num_items == psbt->unknowns.num_items);
  assert(roundTripped->inputs[0].unknowns.num_items == psbt->inputs[0].unknowns.num_items);
  assert(roundTripped->outputs[0].unknowns.num_items == psbt->outputs[0].unknowns.num_items);
  assert(roundTripped->inputs[0].utxo != NULL && roundTripped->inputs[1].utxo != NULL);
  assert(roundTripped->inputs[0].keypaths.num_items == psbt->inputs[0].keypaths.num_items);
  assert(roundTripped->outputs[0].keypaths.num_items == psbt->outputs[0].keypaths.num_items);

  struct wally_psbt *unsupportedSighash = NULL;
  assert(wally_psbt_from_base64_n(encoded.c_str(), encoded.size(),
    WALLY_PSBT_PARSE_FLAG_STRICT, &unsupportedSighash) == WALLY_OK);
  assert(wally_psbt_set_input_sighash(unsupportedSighash, 0,
    WALLY_SIGHASH_ALL | WALLY_SIGHASH_ANYONECANPAY) == WALLY_OK);
  size_t sighash = 0;
  assert(wally_psbt_get_input_sighash(unsupportedSighash, 0, &sighash) == WALLY_OK);
  assert(sighash == (WALLY_SIGHASH_ALL | WALLY_SIGHASH_ANYONECANPAY));

  wally_psbt_free(unsupportedSighash);
  wally_psbt_free(roundTripped);
  wally_free_string(signedBase64);
  wally_psbt_free(psbt);
  wally_bzero(&root, sizeof(root));
  wally_bzero(seed, sizeof(seed));
}

void standardMultisigPsbtSigns() {
  const uint32_t path[] = {
    48 | BIP32_INITIAL_HARDENED_CHILD,
    1 | BIP32_INITIAL_HARDENED_CHILD,
    0 | BIP32_INITIAL_HARDENED_CHILD,
    2 | BIP32_INITIAL_HARDENED_CHILD,
    0,
    0,
  };
  unsigned char seed1[32] = {0}, seed2[32] = {0};
  assert(wally_sha256(reinterpret_cast<const unsigned char *>("bowser signer one"),
                      17, seed1, sizeof(seed1)) == WALLY_OK);
  assert(wally_sha256(reinterpret_cast<const unsigned char *>("bowser signer two"),
                      17, seed2, sizeof(seed2)) == WALLY_OK);
  struct ext_key root1 = {}, root2 = {}, child1 = {}, child2 = {};
  assert(bip32_key_from_seed(seed1, sizeof(seed1), BIP32_VER_TEST_PRIVATE, 0, &root1) == WALLY_OK);
  assert(bip32_key_from_seed(seed2, sizeof(seed2), BIP32_VER_TEST_PRIVATE, 0, &root2) == WALLY_OK);
  assert(bip32_key_from_parent_path(&root1, path, 6, BIP32_FLAG_KEY_PRIVATE, &child1) == WALLY_OK);
  assert(bip32_key_from_parent_path(&root2, path, 6, BIP32_FLAG_KEY_PRIVATE, &child2) == WALLY_OK);

  unsigned char publicKeys[EC_PUBLIC_KEY_LEN * 2] = {0};
  std::memcpy(publicKeys, child1.pub_key, EC_PUBLIC_KEY_LEN);
  std::memcpy(publicKeys + EC_PUBLIC_KEY_LEN, child2.pub_key, EC_PUBLIC_KEY_LEN);
  unsigned char witnessScript[128] = {0};
  size_t witnessScriptLength = 0;
  assert(wally_scriptpubkey_multisig_from_bytes(
    publicKeys, sizeof(publicKeys), 2, WALLY_SCRIPT_MULTISIG_SORTED,
    witnessScript, sizeof(witnessScript), &witnessScriptLength
  ) == WALLY_OK);
  unsigned char inputScript[WALLY_SCRIPTPUBKEY_P2WSH_LEN] = {0};
  size_t inputScriptLength = 0;
  assert(wally_witness_program_from_bytes(
    witnessScript, witnessScriptLength, WALLY_SCRIPT_SHA256,
    inputScript, sizeof(inputScript), &inputScriptLength
  ) == WALLY_OK);
  unsigned char outputScript[WALLY_SCRIPTPUBKEY_P2WPKH_LEN] = {0};
  size_t outputScriptLength = 0;
  assert(wally_witness_program_from_bytes(
    child1.pub_key, EC_PUBLIC_KEY_LEN, WALLY_SCRIPT_HASH160,
    outputScript, sizeof(outputScript), &outputScriptLength
  ) == WALLY_OK);

  struct wally_tx *tx = NULL;
  unsigned char previousTxid[WALLY_TXHASH_LEN] = {0x42};
  assert(wally_tx_init_alloc(WALLY_TX_VERSION_2, 0, 1, 1, &tx) == WALLY_OK);
  assert(wally_tx_add_raw_input(tx, previousTxid, sizeof(previousTxid), 0,
    WALLY_TX_SEQUENCE_FINAL, NULL, 0, NULL, 0) == WALLY_OK);
  assert(wally_tx_add_raw_output(tx, 90000, outputScript, outputScriptLength, 0) == WALLY_OK);
  struct wally_psbt *psbt = NULL;
  assert(wally_psbt_from_tx(tx, WALLY_PSBT_VERSION_0, 0, &psbt) == WALLY_OK);
  struct wally_tx_output *utxo = NULL;
  assert(wally_tx_output_init_alloc(100000, inputScript, inputScriptLength, &utxo) == WALLY_OK);
  assert(wally_psbt_set_input_witness_utxo(psbt, 0, utxo) == WALLY_OK);
  assert(wally_psbt_set_input_witness_script(
    psbt, 0, witnessScript, witnessScriptLength
  ) == WALLY_OK);
  assert(child1.pub_key[0] == 0x02 || child1.pub_key[0] == 0x03);
  assert(wally_map_clear(&psbt->inputs[0].keypaths) == WALLY_OK);
  assert(wally_map_init(2, wally_keypath_public_key_verify,
                        &psbt->inputs[0].keypaths) == WALLY_OK);
  assert(wally_psbt_input_keypath_add(&psbt->inputs[0], child1.pub_key, EC_PUBLIC_KEY_LEN,
    root1.hash160, BIP32_KEY_FINGERPRINT_LEN, path, 6) == WALLY_OK);
  assert(wally_psbt_input_keypath_add(&psbt->inputs[0], child2.pub_key, EC_PUBLIC_KEY_LEN,
    root2.hash160, BIP32_KEY_FINGERPRINT_LEN, path, 6) == WALLY_OK);

  size_t signingLength = 0;
  assert(wally_psbt_get_input_signing_script_len(psbt, 0, &signingLength) == WALLY_OK);
  std::vector<unsigned char> signingScript(signingLength);
  assert(wally_psbt_get_input_signing_script(
    psbt, 0, signingScript.data(), signingScript.size(), &signingLength
  ) == WALLY_OK);
  size_t signingType = WALLY_SCRIPT_TYPE_UNKNOWN;
  assert(wally_scriptpubkey_get_type(
    signingScript.data(), signingLength, &signingType
  ) == WALLY_OK);
  assert(signingType == WALLY_SCRIPT_TYPE_P2WSH);
  assert(wally_psbt_sign_bip32(psbt, &root1, EC_FLAG_GRIND_R) == WALLY_OK);
  size_t signatures = 0;
  assert(wally_psbt_get_input_signatures_size(psbt, 0, &signatures) == WALLY_OK);
  assert(signatures == 1);

  wally_tx_output_free(utxo);
  wally_psbt_free(psbt);
  wally_tx_free(tx);
  wally_bzero(&child2, sizeof(child2));
  wally_bzero(&child1, sizeof(child1));
  wally_bzero(&root2, sizeof(root2));
  wally_bzero(&root1, sizeof(root1));
  wally_bzero(seed2, sizeof(seed2));
  wally_bzero(seed1, sizeof(seed1));
}

}  // namespace

int main() {
  assert(wally_init(0) == WALLY_OK);
  bip39AndBip32Work();
  walletStorageKdfIsCompatible();
  existingTestnetXpubIsCompatible();
  psbtSigningPreservesMetadata();
  standardMultisigPsbtSigns();
  wally_cleanup(0);
  return 0;
}
