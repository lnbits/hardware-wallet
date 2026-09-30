CommandResponse signReviewedPsbt(struct wally_psbt *psbt, struct ext_key *root) {
  // Defend the signature snapshots even when called without policy validation.
  static const size_t MAX_PSBT_INPUTS = 64;
  if (psbt->num_inputs > MAX_PSBT_INPUTS) {
    sendCommandOutput(COMMAND_SIGN_PSBT, "too_many_inputs");
    return {"PSBT unsupported", "Too many inputs"};
  }
  showMessage("Please wait", "Signing PSBT...");
  size_t before[MAX_PSBT_INPUTS] = {0};
  size_t taprootBefore[MAX_PSBT_INPUTS] = {0};
  for (size_t i = 0; i < psbt->num_inputs; i++) {
    wally_psbt_get_input_signatures_size(psbt, i, &before[i]);
    wally_psbt_get_input_taproot_signature_len(psbt, i, &taprootBefore[i]);
  }
  int result = wally_psbt_signing_cache_enable(psbt, 0);
  if (result == WALLY_OK) result = wally_psbt_sign_bip32(psbt, root, EC_FLAG_GRIND_R);
  wally_psbt_signing_cache_disable(psbt);
  if (result != WALLY_OK) {
    sendCommandOutput(COMMAND_SIGN_PSBT, "sign_failed");
    return {"Signing failed", "Invalid signing data"};
  }
  size_t signedInputCount = 0;
  for (size_t i = 0; i < psbt->num_inputs; i++) {
    size_t after = 0;
    size_t taprootAfter = 0;
    const bool addedEcdsa =
      wally_psbt_get_input_signatures_size(psbt, i, &after) == WALLY_OK && after > before[i];
    const bool addedTaproot =
      wally_psbt_get_input_taproot_signature_len(psbt, i, &taprootAfter) == WALLY_OK &&
      taprootAfter > taprootBefore[i];
    if (addedEcdsa || addedTaproot) {
      signedInputCount++;
    }
  }
  if (signedInputCount == 0) {
    sendCommandOutput(COMMAND_SIGN_PSBT, "no_matching_keys");
    return {"No inputs signed", "Keypaths did not match"};
  }
  char *base64 = NULL;
  if (wally_psbt_to_base64(psbt, 0, &base64) != WALLY_OK || base64 == NULL) {
    return {"Signing failed", "Could not serialize PSBT"};
  }
  String signedPsbt(base64);
  wally_free_string(base64);
  sendCommandOutput(COMMAND_SIGN_PSBT, String(signedInputCount) + " " + signedPsbt);
  return {"Signed inputs:", String(signedInputCount)};
}
