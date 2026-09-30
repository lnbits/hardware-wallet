#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>

extern "C" {
#include "libwally.h"
}

// Only the Arduino display/command surface is stubbed; compile the real signer.
class String : public std::string {
 public:
  String(const char *value) : std::string(value) {}
  String(const std::string &value) : std::string(value) {}
  explicit String(size_t value) : std::string(std::to_string(value)) {}
};

struct CommandResponse {
  String message;
  String subMessage;
};

const String COMMAND_SIGN_PSBT = "/sign";
std::string lastCommand;
std::string lastOutput;
size_t displayCalls = 0;

void sendCommandOutput(const String &command, const String &output) {
  lastCommand = command;
  lastOutput = output;
}

void showMessage(const String &, const String &) {
  ++displayCalls;
}

#include "../wallet/719_sign_reviewed_psbt.ino"

std::string serializePsbt(const struct wally_psbt *psbt) {
  char *base64 = NULL;
  assert(wally_psbt_to_base64(psbt, 0, &base64) == WALLY_OK);
  assert(base64 != NULL);
  const std::string result(base64);
  wally_free_string(base64);
  return result;
}

void checkInputLimit(size_t inputCount, uint32_t version, bool taproot) {
  const uint32_t path[] = {
    (taproot ? 86u : 84u) | BIP32_INITIAL_HARDENED_CHILD,
    1 | BIP32_INITIAL_HARDENED_CHILD,
    0 | BIP32_INITIAL_HARDENED_CHILD, 0, 0,
  };
  const unsigned char seed[32] = {1};
  struct ext_key root = {}, child = {};
  assert(bip32_key_from_seed(seed, sizeof(seed), BIP32_VER_TEST_PRIVATE, 0, &root) == WALLY_OK);
  assert(bip32_key_from_parent_path(&root, path, 5, BIP32_FLAG_KEY_PRIVATE, &child) == WALLY_OK);

  unsigned char script[WALLY_SCRIPTPUBKEY_P2TR_LEN] = {0};
  size_t scriptLength = 0;
  if (taproot) {
    unsigned char tweaked[EC_PUBLIC_KEY_LEN] = {0};
    assert(wally_ec_public_key_bip341_tweak(
      child.pub_key, EC_PUBLIC_KEY_LEN, NULL, 0, 0, tweaked, sizeof(tweaked)
    ) == WALLY_OK);
    assert(wally_witness_program_from_bytes_and_version(
      tweaked + 1, EC_XONLY_PUBLIC_KEY_LEN, 1, 0, script, sizeof(script), &scriptLength
    ) == WALLY_OK);
  } else {
    assert(wally_witness_program_from_bytes(
      child.pub_key, EC_PUBLIC_KEY_LEN, WALLY_SCRIPT_HASH160,
      script, sizeof(script), &scriptLength
    ) == WALLY_OK);
  }

  struct wally_tx *tx = NULL;
  assert(wally_tx_init_alloc(WALLY_TX_VERSION_2, 0, inputCount, 1, &tx) == WALLY_OK);
  const unsigned char txid[WALLY_TXHASH_LEN] = {1};
  for (size_t i = 0; i < inputCount; ++i) {
    assert(wally_tx_add_raw_input(tx, txid, sizeof(txid), i,
      WALLY_TX_SEQUENCE_FINAL, NULL, 0, NULL, 0) == WALLY_OK);
  }
  assert(wally_tx_add_raw_output(tx, inputCount * 1000 - 100, script, scriptLength, 0) == WALLY_OK);
  struct wally_psbt *psbt = NULL;
  assert(wally_psbt_from_tx(tx, version, 0, &psbt) == WALLY_OK);
  struct wally_tx_output *utxo = NULL;
  assert(wally_tx_output_init_alloc(1000, script, scriptLength, &utxo) == WALLY_OK);
  for (size_t i = 0; i < inputCount; ++i) {
    assert(wally_psbt_set_input_witness_utxo(psbt, i, utxo) == WALLY_OK);
    if (taproot) {
      assert(wally_psbt_set_input_taproot_internal_key(
        psbt, i, child.pub_key + 1, EC_XONLY_PUBLIC_KEY_LEN
      ) == WALLY_OK);
      assert(wally_map_clear(&psbt->inputs[i].taproot_leaf_paths) == WALLY_OK);
      assert(wally_map_clear(&psbt->inputs[i].taproot_leaf_hashes) == WALLY_OK);
      assert(wally_map_init(1, wally_keypath_xonly_public_key_verify,
        &psbt->inputs[i].taproot_leaf_paths) == WALLY_OK);
      assert(wally_map_init(1, wally_merkle_path_xonly_public_key_verify,
        &psbt->inputs[i].taproot_leaf_hashes) == WALLY_OK);
      assert(wally_psbt_input_taproot_keypath_add(
        &psbt->inputs[i], child.pub_key + 1, EC_XONLY_PUBLIC_KEY_LEN,
        NULL, 0, root.hash160, BIP32_KEY_FINGERPRINT_LEN, path, 5
      ) == WALLY_OK);
    } else {
      assert(wally_map_clear(&psbt->inputs[i].keypaths) == WALLY_OK);
      assert(wally_map_init(1, wally_keypath_public_key_verify,
        &psbt->inputs[i].keypaths) == WALLY_OK);
      assert(wally_psbt_input_keypath_add(
        &psbt->inputs[i], child.pub_key, EC_PUBLIC_KEY_LEN,
        root.hash160, BIP32_KEY_FINGERPRINT_LEN, path, 5
      ) == WALLY_OK);
    }
  }

  const std::string before = serializePsbt(psbt);
  lastCommand.clear();
  lastOutput.clear();
  displayCalls = 0;
  // Deliberately bypass validatePsbtPolicy to exercise the signer's own bound.
  const CommandResponse response = signReviewedPsbt(psbt, &root);
  assert(lastCommand == COMMAND_SIGN_PSBT);
  if (inputCount <= 64) {
    assert(response.message == "Signed inputs:");
    assert(response.subMessage == std::to_string(inputCount));
    assert(lastOutput == std::to_string(inputCount) + " " + serializePsbt(psbt));
    for (size_t i = 0; i < inputCount; ++i) {
      size_t signatures = 0;
      if (taproot) {
        assert(wally_psbt_get_input_taproot_signature_len(psbt, i, &signatures) == WALLY_OK);
        assert(signatures == EC_SIGNATURE_LEN);
      } else {
        assert(wally_psbt_get_input_signatures_size(psbt, i, &signatures) == WALLY_OK);
        assert(signatures == 1);
      }
    }
  } else {
    assert(response.message == "PSBT unsupported");
    assert(response.subMessage == "Too many inputs");
    assert(lastOutput == "too_many_inputs");
    assert(displayCalls == 0);
    assert(serializePsbt(psbt) == before);
  }
  wally_tx_output_free(utxo);
  wally_psbt_free(psbt);
  wally_tx_free(tx);
  wally_bzero(&child, sizeof(child));
  wally_bzero(&root, sizeof(root));
}

int main() {
  assert(wally_init(0) == WALLY_OK);
  for (uint32_t version : {WALLY_PSBT_VERSION_0, WALLY_PSBT_VERSION_2}) {
    for (bool taproot : {false, true}) {
      for (size_t inputCount : {63, 64, 65, 100}) {
        checkInputLimit(inputCount, version, taproot);
      }
    }
  }
  assert(wally_cleanup(0) == WALLY_OK);
  std::cout << "PSBT signing limits passed: 63/64 accepted, 65/100 rejected (v0/v2, ECDSA/Taproot)\n";
}
