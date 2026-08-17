/** Sign a PSBT only after every output and the fee are approved physically. */
CommandResponse executeSignPsbt(String commandData) {
  if (!global.authenticated) return {"Enter password!", "8 numbers/letters"};
  showMessage("Please wait", "Parsing PSBT...");
  int spacePos = commandData.indexOf(" ");
  if (spacePos <= 0) return {"Invalid request", "Network and PSBT required"};
  String networkName = commandData.substring(0, spacePos);
  String psbtBase64 = commandData.substring(spacePos + 1);
  if (psbtBase64.length() == 0 || psbtBase64.length() > 65536) {
    return {"Invalid PSBT", "PSBT is empty or too large"};
  }
  bool mainnet = false;
  if (!isMainnetName(networkName, &mainnet)) {
    return {"Unknown Network", "Must be Mainnet or Testnet"};
  }

  struct wally_psbt *psbt = NULL;
  if (wally_psbt_from_base64_n(psbtBase64.c_str(), psbtBase64.length(),
        WALLY_PSBT_PARSE_FLAG_STRICT, &psbt) != WALLY_OK || psbt == NULL) {
    sendCommandOutput(COMMAND_SEND_PSBT, "psbt_parse_failed");
    return {"Failed parsing", "Send PSBT again"};
  }
  String policyReason;
  if (!validatePsbtPolicy(psbt, &policyReason)) {
    wally_psbt_free(psbt);
    sendCommandOutput(COMMAND_SEND_PSBT, "psbt_unsupported");
    return {"PSBT unsupported", policyReason};
  }
  struct ext_key root = {};
  if (!mnemonicIsValid(global.mnemonic) || !deriveRootKey(mainnet, &root)) {
    wally_psbt_free(psbt);
    sendCommandOutput(COMMAND_SEND_PSBT, "invalid_mnemonic");
    return {"Invalid Mnemonic", ""};
  }
  if (!validatePsbtNetworkKeypaths(psbt, &root, mainnet)) {
    clearSensitiveBytes((uint8_t *)&root, sizeof(root));
    wally_psbt_free(psbt);
    sendCommandOutput(COMMAND_SEND_PSBT, "psbt_network_mismatch");
    return {"PSBT rejected", "Network/path mismatch"};
  }
  uint64_t fee = 0;
  if (!calculatePsbtFee(psbt, &fee)) {
    clearSensitiveBytes((uint8_t *)&root, sizeof(root));
    wally_psbt_free(psbt);
    sendCommandOutput(COMMAND_SEND_PSBT, "psbt_invalid_amounts");
    return {"Invalid PSBT", "Could not verify fee"};
  }
  if (global.hasCommandsFile) sendCommandOutput(COMMAND_SEND_PSBT, "1");
  CommandResponse response = reviewAndSignPsbt(psbt, &root, mainnet, fee);
  clearSensitiveBytes((uint8_t *)&root, sizeof(root));
  wally_psbt_free(psbt);
  return response;
}

CommandResponse reviewAndSignPsbt(struct wally_psbt *psbt, struct ext_key *root,
                                  bool mainnet, uint64_t fee) {
  for (size_t i = 0; i < psbt->num_outputs; i++) {
    if (global.hasCommandsFile) writeOutputToFile(psbt, i, mainnet);
    printOutputDetails(psbt, root, i, mainnet);
    printPhysicalReviewControls();
    if (!awaitPhysicalReviewApproval()) {
      sendCommandOutput(COMMAND_SEND_PSBT, "review_rejected");
      return {"Operation canceled", "Output rejected"};
    }
  }
  if (global.hasCommandsFile) writeFeeToFile(fee);
  printFeeDetails(fee);
  printPhysicalReviewControls();
  if (!awaitPhysicalReviewApproval()) {
    sendCommandOutput(COMMAND_SEND_PSBT, "review_rejected");
    return {"Operation canceled", "Fee rejected"};
  }
  if (!global.hasCommandsFile) sendCommandOutput(COMMAND_SEND_PSBT, "1");

  if (global.hasCommandsFile) {
    showConfirmCancelMessage("Sign reviewed PSBT?", "Review complete");
    if (!awaitPhysicalReviewApproval()) {
      sendCommandOutput(COMMAND_SEND_PSBT, "review_rejected");
      return {"Operation canceled", "Signing rejected"};
    }
    return signReviewedPsbt(psbt, root);
  }
  while (true) {
    EventData event = awaitEvent();
    if (event.type != EVENT_SERIAL_DATA) continue;
    Command command = decryptAndExtractCommand(event.data);
    // Kept solely for older clients; serial data can never advance review.
    if (command.cmd == COMMAND_CONFIRM_NEXT) continue;
    if (command.cmd == COMMAND_CANCEL) return {"Operation Canceled", "`/help` for details"};
    if (command.cmd != COMMAND_SIGN_PSBT) return executeUnknown("Expected: " + COMMAND_SIGN_PSBT);
    showConfirmCancelMessage("Confirm signing", "Outputs and fee reviewed");
    if (!awaitPhysicalReviewApproval()) {
      sendCommandOutput(COMMAND_SIGN_PSBT, "review_rejected");
      return {"Operation Canceled", "Signing rejected"};
    }
    return signReviewedPsbt(psbt, root);
  }
}

void writeOutputToFile(const struct wally_psbt *psbt, size_t index, bool mainnet) {
  uint64_t amount = 0;
  wally_psbt_get_output_amount(psbt, index, &amount);
  commandOutToFile("Output " + String(index) + "\n" +
    "Address: " + psbtOutputDescription(psbt, index, mainnet) + "\n" +
    "Amount: " + int64ToString(amount) + " sat");
}

void writeFeeToFile(uint64_t fee) {
  commandOutToFile("Fee: " + int64ToString(fee) + " sat");
}

CommandResponse signReviewedPsbt(struct wally_psbt *psbt, struct ext_key *root) {
  showMessage("Please wait", "Signing PSBT...");
  size_t before[64] = {0};
  for (size_t i = 0; i < psbt->num_inputs; i++) {
    wally_psbt_get_input_signatures_size(psbt, i, &before[i]);
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
    if (wally_psbt_get_input_signatures_size(psbt, i, &after) == WALLY_OK && after > before[i]) {
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
