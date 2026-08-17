const size_t BOWSER_PSBT_MAX_REDEEM_SCRIPT_LEN = 520;

bool readPsbtOutput(const struct wally_psbt *psbt, size_t index,
                    uint8_t *script, size_t capacity, size_t *scriptLength,
                    uint64_t *amount) {
  return wally_psbt_get_output_script(psbt, index, script, capacity, scriptLength) == WALLY_OK &&
         wally_psbt_get_output_amount(psbt, index, amount) == WALLY_OK;
}

String psbtOutputDescription(const struct wally_psbt *psbt, size_t index, bool mainnet) {
  uint8_t script[WALLY_SCRIPTPUBKEY_OP_RETURN_MAX_LEN] = {0};
  size_t scriptLength = 0;
  uint64_t amount = 0;
  if (!readPsbtOutput(psbt, index, script, sizeof(script), &scriptLength, &amount)) return "";
  size_t scriptType = WALLY_SCRIPT_TYPE_UNKNOWN;
  if (wally_scriptpubkey_get_type(script, scriptLength, &scriptType) != WALLY_OK) return "";
  if (scriptType == WALLY_SCRIPT_TYPE_OP_RETURN) return "OP_RETURN data";
  return addressFromScript(script, scriptLength, mainnet);
}

bool psbtOutputIsSingleSigChange(const struct wally_psbt *psbt, size_t index,
                                 const struct ext_key *root) {
  if (index >= psbt->num_outputs || psbt->outputs[index].keypaths.num_items != 1) return false;
  struct ext_key derived = {};
  size_t found = 0;
  if (wally_map_keypath_get_bip32_public_key_from(
        &psbt->outputs[index].keypaths, 0, root, &derived, &found) != WALLY_OK || found == 0) {
    clearSensitiveBytes((uint8_t *)&derived, sizeof(derived));
    return false;
  }

  uint8_t actual[WALLY_SCRIPTPUBKEY_OP_RETURN_MAX_LEN] = {0};
  size_t actualLength = 0;
  uint64_t ignoredAmount = 0;
  if (!readPsbtOutput(psbt, index, actual, sizeof(actual), &actualLength, &ignoredAmount)) {
    clearSensitiveBytes((uint8_t *)&derived, sizeof(derived));
    return false;
  }
  size_t scriptType = WALLY_SCRIPT_TYPE_UNKNOWN;
  wally_scriptpubkey_get_type(actual, actualLength, &scriptType);
  uint8_t expected[WALLY_SCRIPTPUBKEY_P2PKH_LEN] = {0};
  size_t expectedLength = 0;
  bool built = false;
  if (scriptType == WALLY_SCRIPT_TYPE_P2PKH) {
    built = wally_scriptpubkey_p2pkh_from_bytes(
      derived.pub_key, EC_PUBLIC_KEY_LEN, WALLY_SCRIPT_HASH160,
      expected, sizeof(expected), &expectedLength) == WALLY_OK;
  } else {
    uint8_t witness[WALLY_SCRIPTPUBKEY_P2WPKH_LEN] = {0};
    size_t witnessLength = 0;
    if (wally_witness_program_from_bytes(
          derived.pub_key, EC_PUBLIC_KEY_LEN, WALLY_SCRIPT_HASH160,
          witness, sizeof(witness), &witnessLength) == WALLY_OK) {
      if (scriptType == WALLY_SCRIPT_TYPE_P2WPKH) {
        memcpy(expected, witness, witnessLength);
        expectedLength = witnessLength;
        built = true;
      } else if (scriptType == WALLY_SCRIPT_TYPE_P2SH) {
        uint8_t redeem[WALLY_SCRIPTPUBKEY_P2WPKH_LEN] = {0};
        size_t redeemLength = 0;
        built = wally_psbt_get_output_redeem_script(
                  psbt, index, redeem, sizeof(redeem), &redeemLength) == WALLY_OK &&
                redeemLength == witnessLength && memcmp(redeem, witness, witnessLength) == 0 &&
                wally_scriptpubkey_p2sh_from_bytes(
                  witness, witnessLength, WALLY_SCRIPT_HASH160,
                  expected, sizeof(expected), &expectedLength) == WALLY_OK;
      }
    }
  }
  const bool matches = built && expectedLength == actualLength &&
                       memcmp(expected, actual, actualLength) == 0;
  clearSensitiveBytes((uint8_t *)&derived, sizeof(derived));
  clearSensitiveBytes(expected, sizeof(expected));
  return matches;
}

void printOutputDetails(const struct wally_psbt *psbt, const struct ext_key *root,
                        int index, bool mainnet) {
  beginUiScreen(UiControls::ConfirmCancel);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(4, 2);
  String heading = "Output " + String(index);
  tft.setTextSize(uiFittedTextSize(heading, 2, tft.width() - 8));
  tft.println(heading);
  tft.setTextSize(uiTextSize(1));
  tft.println("");
  if (psbtOutputIsSingleSigChange(psbt, index, root)) tft.print("Change ");
  tft.println("Address:");
  tft.println("");
  String description = psbtOutputDescription(psbt, index, mainnet);
  tft.setTextSize(uiFittedTextSize(description, 1, tft.width() - 8));
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.println(description);
  tft.setTextSize(uiTextSize(1));
  uint64_t amount = 0;
  wally_psbt_get_output_amount(psbt, index, &amount);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.println("");
  tft.println("Amount:");
  tft.println("");
  tft.setTextSize(uiTextSize(2));
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  printSats(int64ToString(amount), uiTextSize(2));
  tft.println(" sat");
}

bool validatePsbtPolicy(const struct wally_psbt *psbt, String *reason) {
  if (psbt == NULL || psbt->version != WALLY_PSBT_VERSION_0) {
    *reason = "Only PSBT v0 supported";
    return false;
  }
  if (psbt->num_inputs == 0 || psbt->num_inputs > 64 ||
      psbt->num_outputs == 0 || psbt->num_outputs > 64) {
    *reason = "Invalid input/output count";
    return false;
  }
  for (size_t i = 0; i < psbt->num_inputs; i++) {
    const struct wally_psbt_input *input = &psbt->inputs[i];
    if (input->sighash != 0 && input->sighash != WALLY_SIGHASH_ALL) {
      *reason = "Unsupported sighash";
      return false;
    }
    size_t finalScriptLength = 0;
    if (input->final_witness != NULL ||
        wally_psbt_get_input_final_scriptsig_len(psbt, i, &finalScriptLength) != WALLY_OK ||
        finalScriptLength != 0) {
      *reason = "Finalized input unsupported";
      return false;
    }
    if (input->keypaths.num_items == 0) {
      *reason = "Input keypath missing";
      return false;
    }
    const struct wally_tx_output *utxo = NULL;
    if (wally_psbt_get_input_best_utxo(psbt, i, &utxo) != WALLY_OK || utxo == NULL) {
      *reason = "Input UTXO missing";
      return false;
    }
    uint8_t script[WALLY_SCRIPTPUBKEY_OP_RETURN_MAX_LEN] = {0};
    size_t scriptLength = 0, scriptType = WALLY_SCRIPT_TYPE_UNKNOWN;
    if (wally_tx_output_get_script(utxo, script, sizeof(script), &scriptLength) != WALLY_OK ||
        wally_scriptpubkey_get_type(script, scriptLength, &scriptType) != WALLY_OK ||
        (scriptType != WALLY_SCRIPT_TYPE_P2PKH && scriptType != WALLY_SCRIPT_TYPE_P2SH &&
         scriptType != WALLY_SCRIPT_TYPE_P2WPKH && scriptType != WALLY_SCRIPT_TYPE_P2WSH)) {
      *reason = "Unsupported input script";
      return false;
    }
    if (scriptType == WALLY_SCRIPT_TYPE_P2SH || scriptType == WALLY_SCRIPT_TYPE_P2WSH) {
      uint8_t redeem[BOWSER_PSBT_MAX_REDEEM_SCRIPT_LEN] = {0};
      size_t redeemLength = 0, redeemType = WALLY_SCRIPT_TYPE_UNKNOWN;
      const bool hasRedeem = scriptType == WALLY_SCRIPT_TYPE_P2SH &&
        wally_psbt_get_input_redeem_script(psbt, i, redeem, sizeof(redeem), &redeemLength) == WALLY_OK &&
        redeemLength != 0 && wally_scriptpubkey_get_type(redeem, redeemLength, &redeemType) == WALLY_OK;
      if (scriptType == WALLY_SCRIPT_TYPE_P2SH && hasRedeem &&
          (redeemType == WALLY_SCRIPT_TYPE_P2WPKH || redeemType == WALLY_SCRIPT_TYPE_MULTISIG)) continue;
      const bool wrappedWitness = scriptType == WALLY_SCRIPT_TYPE_P2WSH ||
                                  (hasRedeem && redeemType == WALLY_SCRIPT_TYPE_P2WSH);
      uint8_t witness[BOWSER_PSBT_MAX_REDEEM_SCRIPT_LEN] = {0};
      size_t witnessLength = 0, witnessType = WALLY_SCRIPT_TYPE_UNKNOWN;
      if (!wrappedWitness ||
          wally_psbt_get_input_witness_script(psbt, i, witness, sizeof(witness), &witnessLength) != WALLY_OK ||
          witnessLength == 0 || wally_scriptpubkey_get_type(witness, witnessLength, &witnessType) != WALLY_OK ||
          witnessType != WALLY_SCRIPT_TYPE_MULTISIG) {
        *reason = "Unsupported redeem script";
        return false;
      }
    }

    // This libwally call validates that non-witness UTXOs match the prevout
    // txid and that redeem/witness scripts hash to the advertised UTXO. Do it
    // before displaying amounts so an untrusted coordinator cannot spoof the
    // fee review with inconsistent metadata.
    uint8_t signingScript[BOWSER_PSBT_MAX_REDEEM_SCRIPT_LEN] = {0};
    size_t signingScriptLength = 0, signingScriptType = WALLY_SCRIPT_TYPE_UNKNOWN;
    if (wally_psbt_get_input_signing_script_len(psbt, i, &signingScriptLength) != WALLY_OK ||
        signingScriptLength == 0 || signingScriptLength > sizeof(signingScript) ||
        wally_psbt_get_input_signing_script(
          psbt, i, signingScript, sizeof(signingScript), &signingScriptLength) != WALLY_OK ||
        wally_scriptpubkey_get_type(
          signingScript, signingScriptLength, &signingScriptType) != WALLY_OK ||
        (signingScriptType != WALLY_SCRIPT_TYPE_P2PKH &&
         signingScriptType != WALLY_SCRIPT_TYPE_P2WPKH &&
         signingScriptType != WALLY_SCRIPT_TYPE_P2WSH &&
         signingScriptType != WALLY_SCRIPT_TYPE_MULTISIG)) {
      *reason = "Invalid signing metadata";
      return false;
    }
  }
  for (size_t i = 0; i < psbt->num_outputs; i++) {
    uint8_t script[WALLY_SCRIPTPUBKEY_OP_RETURN_MAX_LEN] = {0};
    size_t scriptLength = 0, scriptType = WALLY_SCRIPT_TYPE_UNKNOWN;
    uint64_t amount = 0;
    if (!readPsbtOutput(psbt, i, script, sizeof(script), &scriptLength, &amount) ||
        wally_scriptpubkey_get_type(script, scriptLength, &scriptType) != WALLY_OK ||
        (scriptType != WALLY_SCRIPT_TYPE_P2PKH && scriptType != WALLY_SCRIPT_TYPE_P2SH &&
         scriptType != WALLY_SCRIPT_TYPE_P2WPKH && scriptType != WALLY_SCRIPT_TYPE_P2WSH)) {
      *reason = "Unsupported output script";
      return false;
    }
  }
  return true;
}

bool validatePsbtNetworkKeypaths(const struct wally_psbt *psbt,
                                 const struct ext_key *root, bool mainnet) {
  const uint32_t expectedCoin = BIP32_INITIAL_HARDENED_CHILD + (mainnet ? 0 : 1);
  bool foundWalletKey = false;
  for (size_t inputIndex = 0; inputIndex < psbt->num_inputs; inputIndex++) {
    const struct wally_map *keypaths = &psbt->inputs[inputIndex].keypaths;
    size_t start = 0;
    while (start < keypaths->num_items) {
      struct ext_key derived = {};
      size_t found = 0;
      const int result = wally_map_keypath_get_bip32_public_key_from(
        keypaths, start, root, &derived, &found
      );
      clearSensitiveBytes((uint8_t *)&derived, sizeof(derived));
      if (result != WALLY_OK) return false;
      if (found == 0) break;
      uint32_t path[BIP32_PATH_MAX_LEN] = {0};
      size_t pathLength = 0;
      if (wally_map_keypath_get_item_path(
            keypaths, found - 1, path, BIP32_PATH_MAX_LEN, &pathLength
          ) != WALLY_OK || pathLength < 2 || path[1] != expectedCoin ||
          (path[0] != BIP32_INITIAL_HARDENED_CHILD + 44 &&
           path[0] != BIP32_INITIAL_HARDENED_CHILD + 48 &&
           path[0] != BIP32_INITIAL_HARDENED_CHILD + 49 &&
           path[0] != BIP32_INITIAL_HARDENED_CHILD + 84)) {
        clearSensitiveBytes((uint8_t *)path, sizeof(path));
        return false;
      }
      clearSensitiveBytes((uint8_t *)path, sizeof(path));
      foundWalletKey = true;
      start = found;
    }
  }
  return foundWalletKey;
}

bool calculatePsbtFee(const struct wally_psbt *psbt, uint64_t *fee) {
  uint64_t inputs = 0, outputs = 0;
  for (size_t i = 0; i < psbt->num_inputs; i++) {
    const struct wally_tx_output *utxo = NULL;
    uint64_t amount = 0;
    if (wally_psbt_get_input_best_utxo(psbt, i, &utxo) != WALLY_OK || utxo == NULL ||
        wally_tx_output_get_satoshi(utxo, &amount) != WALLY_OK || UINT64_MAX - inputs < amount) return false;
    inputs += amount;
  }
  for (size_t i = 0; i < psbt->num_outputs; i++) {
    uint64_t amount = 0;
    if (wally_psbt_get_output_amount(psbt, i, &amount) != WALLY_OK || UINT64_MAX - outputs < amount) return false;
    outputs += amount;
  }
  if (outputs > inputs) return false;
  *fee = inputs - outputs;
  return true;
}

void printFeeDetails(uint64_t fee) {
  beginUiScreen(UiControls::ConfirmCancel);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(4, max(8, uiContentHeight() / 3));
  String feeLabel = "Fee: " + int64ToString(fee) + " sat";
  tft.setTextSize(uiFittedTextSize(feeLabel, 2, tft.width() - 8));
  tft.print("Fee: ");
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  printSats(int64ToString(fee), uiFittedTextSize(feeLabel, 2, tft.width() - 8));
  tft.println(" sat");
}

void printPhysicalReviewControls() {
  if (BOARD.hasTouchscreen) return;
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(uiTextSize(1));
  tft.setCursor(0, tft.height() - 11);
  if (BOARD.singleButtonLongPressCancels) tft.print("Tap accept / hold cancel");
  else tft.print("# / BTN1 accept   * / BTN2 cancel");
}

void printSats(String sats, int textSize) {
  int len = sats.length(), offest = len % 3;
  for (int i = 0; i < len; i++) {
    tft.print(sats[i]);
    if ((i + 1 - offest) % 3 == 0) {
      tft.setTextSize(uiTextSize(1));
      tft.print(" ");
      tft.setTextSize(textSize);
    }
  }
}
