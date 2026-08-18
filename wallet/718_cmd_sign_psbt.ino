const size_t BOWSER_PSBT_MAX_BASE64_LENGTH = 65536;
const size_t BOWSER_PSBT_SERIAL_CHUNK_LENGTH = 64;
const unsigned long BOWSER_PSBT_TRANSFER_TIMEOUT_MS = 2UL * 60UL * 1000UL;

String pendingPsbtNetwork = "";
String pendingPsbtBase64 = "";
size_t pendingPsbtExpectedLength = 0;
size_t pendingPsbtNextChunk = 0;
unsigned long pendingPsbtStartedAt = 0;

void clearPendingPsbtTransfer() {
  pendingPsbtNetwork = "";
  pendingPsbtBase64 = "";
  pendingPsbtExpectedLength = 0;
  pendingPsbtNextChunk = 0;
  pendingPsbtStartedAt = 0;
}

bool parsePsbtSizeValue(const String &value, size_t *result) {
  if (value.length() == 0) return false;
  size_t parsed = 0;
  for (size_t i = 0; i < value.length(); i++) {
    const char c = value[i];
    if (c < '0' || c > '9') return false;
    const size_t digit = (size_t)(c - '0');
    if (parsed > (SIZE_MAX - digit) / 10) return false;
    parsed = parsed * 10 + digit;
  }
  *result = parsed;
  return true;
}

bool isPsbtBase64Chunk(const String &chunk) {
  for (size_t i = 0; i < chunk.length(); i++) {
    const char c = chunk[i];
    if (!((c >= 'A' && c <= 'Z') ||
          (c >= 'a' && c <= 'z') ||
          (c >= '0' && c <= '9') ||
          c == '+' || c == '/' || c == '=')) return false;
  }
  return true;
}

void sendPsbtTransferResponse(const String &command, const String &data) {
  // These acknowledgements contain only status and indexes. Keeping them
  // plaintext avoids running the hardware RNG once for every 64-byte chunk.
  Serial.println(command + " " + data);
}

CommandResponse rejectPsbtTransfer(const String &command,
                                   const String &code,
                                   const String &message) {
  sendPsbtTransferResponse(command, "0 " + code);
  return {"PSBT transfer failed", message};
}

CommandResponse executePsbtTransferBegin(String commandData) {
  clearPendingPsbtTransfer();
  if (global.hasCommandsFile) {
    return {"Unsupported command", "Chunking is WebSerial only"};
  }
  if (!global.authenticated) {
    return rejectPsbtTransfer(COMMAND_PSBT_BEGIN, "locked", "Unlock wallet first");
  }
  const int separator = commandData.indexOf(' ');
  if (separator <= 0 || commandData.indexOf(' ', separator + 1) != -1) {
    return rejectPsbtTransfer(COMMAND_PSBT_BEGIN, "invalid_request", "Network and length required");
  }
  const String networkName = commandData.substring(0, separator);
  bool mainnet = false;
  if (!isMainnetName(networkName, &mainnet)) {
    return rejectPsbtTransfer(COMMAND_PSBT_BEGIN, "invalid_network", "Unknown network");
  }
  size_t expectedLength = 0;
  if (!parsePsbtSizeValue(commandData.substring(separator + 1), &expectedLength) ||
      expectedLength == 0 || expectedLength > BOWSER_PSBT_MAX_BASE64_LENGTH) {
    return rejectPsbtTransfer(COMMAND_PSBT_BEGIN, "invalid_length", "PSBT is empty or too large");
  }
  if (!pendingPsbtBase64.reserve(expectedLength)) {
    clearPendingPsbtTransfer();
    return rejectPsbtTransfer(COMMAND_PSBT_BEGIN, "no_memory", "Could not reserve PSBT buffer");
  }
  pendingPsbtNetwork = networkName;
  pendingPsbtExpectedLength = expectedLength;
  pendingPsbtStartedAt = millis();
  const size_t chunkCount =
    (expectedLength + BOWSER_PSBT_SERIAL_CHUNK_LENGTH - 1) /
    BOWSER_PSBT_SERIAL_CHUNK_LENGTH;
  sendPsbtTransferResponse(COMMAND_PSBT_BEGIN, "1 " + String(chunkCount));
  showMessage("Receiving PSBT", String(chunkCount) + " chunks");
  return {"", ""};
}

CommandResponse executePsbtTransferChunk(String commandData) {
  if (global.hasCommandsFile) {
    return {"Unsupported command", "Chunking is WebSerial only"};
  }
  if (pendingPsbtExpectedLength == 0 ||
      millis() - pendingPsbtStartedAt > BOWSER_PSBT_TRANSFER_TIMEOUT_MS) {
    clearPendingPsbtTransfer();
    return rejectPsbtTransfer(COMMAND_PSBT_CHUNK, "no_transfer", "Start PSBT transfer again");
  }
  const int separator = commandData.indexOf(' ');
  if (separator <= 0) {
    clearPendingPsbtTransfer();
    return rejectPsbtTransfer(COMMAND_PSBT_CHUNK, "invalid_request", "Chunk index and data required");
  }
  size_t index = 0;
  if (!parsePsbtSizeValue(commandData.substring(0, separator), &index) ||
      index != pendingPsbtNextChunk) {
    clearPendingPsbtTransfer();
    return rejectPsbtTransfer(COMMAND_PSBT_CHUNK, "wrong_index", "PSBT chunks arrived out of order");
  }
  const String chunk = commandData.substring(separator + 1);
  const size_t remaining = pendingPsbtExpectedLength - pendingPsbtBase64.length();
  const size_t expectedChunkLength =
    remaining < BOWSER_PSBT_SERIAL_CHUNK_LENGTH ? remaining : BOWSER_PSBT_SERIAL_CHUNK_LENGTH;
  if (chunk.length() != expectedChunkLength || !isPsbtBase64Chunk(chunk) ||
      !pendingPsbtBase64.concat(chunk)) {
    clearPendingPsbtTransfer();
    return rejectPsbtTransfer(COMMAND_PSBT_CHUNK, "invalid_chunk", "Invalid PSBT chunk");
  }
  pendingPsbtNextChunk++;
  pendingPsbtStartedAt = millis();
  sendPsbtTransferResponse(COMMAND_PSBT_CHUNK, "1 " + String(index));
  // Keep the first transfer screen in place. Redrawing it for every chunk
  // caused visible flicker, particularly on the C6 display.
  return {"", ""};
}

CommandResponse executePsbtTransferCommit(String commandData) {
  if (global.hasCommandsFile) {
    return {"Unsupported command", "Chunking is WebSerial only"};
  }
  commandData.trim();
  const size_t expectedChunks =
    (pendingPsbtExpectedLength + BOWSER_PSBT_SERIAL_CHUNK_LENGTH - 1) /
    BOWSER_PSBT_SERIAL_CHUNK_LENGTH;
  if (commandData.length() != 0 || pendingPsbtExpectedLength == 0 ||
      pendingPsbtBase64.length() != pendingPsbtExpectedLength ||
      pendingPsbtNextChunk != expectedChunks ||
      millis() - pendingPsbtStartedAt > BOWSER_PSBT_TRANSFER_TIMEOUT_MS) {
    clearPendingPsbtTransfer();
    sendCommandOutput(COMMAND_PSBT_COMMIT, "incomplete_transfer");
    return {"PSBT transfer failed", "Incomplete PSBT"};
  }
  CommandResponse response = executeSignPsbtPayload(
    pendingPsbtNetwork,
    pendingPsbtBase64,
    COMMAND_PSBT_COMMIT
  );
  clearPendingPsbtTransfer();
  return response;
}

/** Parse and sign a PSBT received in one legacy WebSerial command. */
CommandResponse executeSignPsbt(String commandData) {
  int spacePos = commandData.indexOf(" ");
  if (spacePos <= 0) {
    sendCommandOutput(COMMAND_SEND_PSBT, "invalid_request");
    return {"Invalid request", "Network and PSBT required"};
  }
  String networkName = commandData.substring(0, spacePos);
  String psbtBase64 = commandData.substring(spacePos + 1);
  return executeSignPsbtPayload(networkName, psbtBase64, COMMAND_SEND_PSBT);
}

CommandResponse executeSignPsbtPayload(const String &networkName,
                                       const String &psbtBase64,
                                       const String &responseCommand) {
  if (!global.authenticated) {
    sendCommandOutput(responseCommand, "locked");
    return {"Enter password!", "8 numbers/letters"};
  }
  showMessage("Please wait", "Parsing PSBT...");
  if (psbtBase64.length() == 0 || psbtBase64.length() > BOWSER_PSBT_MAX_BASE64_LENGTH) {
    sendCommandOutput(responseCommand, "invalid_length");
    return {"Invalid PSBT", "PSBT is empty or too large"};
  }
  bool mainnet = false;
  if (!isMainnetName(networkName, &mainnet)) {
    sendCommandOutput(responseCommand, "invalid_network");
    return {"Unknown Network", "Must be Mainnet or Testnet"};
  }

  struct wally_psbt *psbt = NULL;
  if (wally_psbt_from_base64_n(psbtBase64.c_str(), psbtBase64.length(),
        WALLY_PSBT_PARSE_FLAG_STRICT, &psbt) != WALLY_OK || psbt == NULL) {
    sendCommandOutput(responseCommand, "psbt_parse_failed");
    return {"Failed parsing", "Send PSBT again"};
  }
  String policyReason;
  if (!validatePsbtPolicy(psbt, &policyReason)) {
    wally_psbt_free(psbt);
    // Policy reasons are fixed firmware strings and contain no PSBT data.
    // Return one to WebSerial/SD so a supported-flow regression is
    // distinguishable from an intentionally unsupported script type.
    sendCommandOutput(responseCommand, "psbt_unsupported " + policyReason);
    return {"PSBT unsupported", policyReason};
  }
  struct ext_key root = {};
  if (!mnemonicIsValid(global.mnemonic) || !deriveRootKey(mainnet, &root)) {
    wally_psbt_free(psbt);
    sendCommandOutput(responseCommand, "invalid_mnemonic");
    return {"Invalid Mnemonic", ""};
  }
  if (!validatePsbtNetworkKeypaths(psbt, &root, mainnet)) {
    clearSensitiveBytes((uint8_t *)&root, sizeof(root));
    wally_psbt_free(psbt);
    sendCommandOutput(responseCommand, "psbt_network_mismatch");
    return {"PSBT rejected", "Network/path mismatch"};
  }
  uint64_t fee = 0;
  if (!calculatePsbtFee(psbt, &fee)) {
    clearSensitiveBytes((uint8_t *)&root, sizeof(root));
    wally_psbt_free(psbt);
    sendCommandOutput(responseCommand, "psbt_invalid_amounts");
    return {"Invalid PSBT", "Could not verify fee"};
  }
  // SD signing is a single command, so preserve its acknowledgement before
  // the physical review. WebSerial keeps the request pending until the device
  // has physically approved every output and the fee.
  if (global.hasCommandsFile) sendCommandOutput(responseCommand, "1");
  CommandResponse response = reviewAndSignPsbt(
    psbt, &root, mainnet, fee, responseCommand
  );
  clearSensitiveBytes((uint8_t *)&root, sizeof(root));
  wally_psbt_free(psbt);
  return response;
}

CommandResponse reviewAndSignPsbt(struct wally_psbt *psbt, struct ext_key *root,
                                  bool mainnet, uint64_t fee,
                                  const String &reviewResponseCommand) {
  for (size_t i = 0; i < psbt->num_outputs; i++) {
    if (global.hasCommandsFile) writeOutputToFile(psbt, i, mainnet);
    printOutputDetails(psbt, root, i, mainnet);
    printPhysicalReviewControls();
    if (!awaitPhysicalReviewApproval()) {
      sendCommandOutput(reviewResponseCommand, "review_rejected");
      return {"Operation canceled", "Output rejected"};
    }
  }
  if (global.hasCommandsFile) writeFeeToFile(fee);
  printFeeDetails(fee);
  printPhysicalReviewControls();
  if (!awaitPhysicalReviewApproval()) {
    sendCommandOutput(reviewResponseCommand, "review_rejected");
    return {"Operation canceled", "Fee rejected"};
  }

  if (global.hasCommandsFile) {
    showConfirmCancelMessage("Sign reviewed PSBT?", "Review complete");
    if (!awaitPhysicalReviewApproval()) {
      sendCommandOutput(COMMAND_SEND_PSBT, "review_rejected");
      return {"Operation canceled", "Signing rejected"};
    }
    return signReviewedPsbt(psbt, root);
  }

  // Only acknowledge the WebSerial PSBT after its trusted-display review is
  // complete. The client can then request signing, but cannot advance any of
  // the output or fee screens.
  sendCommandOutput(reviewResponseCommand, "1");
  while (true) {
    EventData event = awaitEvent();
    if (event.type != EVENT_SERIAL_DATA) continue;
    Command command = decryptAndExtractCommand(event.data);
    // Older clients may still send this command. It has no approval meaning
    // and is ignored solely so they cannot cause an unknown-command failure.
    if (command.cmd == COMMAND_CONFIRM_NEXT) continue;
    if (command.cmd == COMMAND_CANCEL) {
      return {"Operation canceled", "`/help` for details"};
    }
    if (command.cmd != COMMAND_SIGN_PSBT) {
      return executeUnknown("Expected: " + COMMAND_SIGN_PSBT);
    }
    showConfirmCancelMessage("Confirm signing", "Outputs and fee reviewed");
    if (!awaitPhysicalReviewApproval()) {
      sendCommandOutput(COMMAND_SIGN_PSBT, "review_rejected");
      return {"Operation canceled", "Signing rejected"};
    }
    return signReviewedPsbt(psbt, root);
  }
}

void writeOutputToFile(const struct wally_psbt *psbt, size_t index, bool mainnet) {
  uint64_t amount = 0;
  readPsbtOutputAmount(psbt, index, &amount);
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
  size_t taprootBefore[64] = {0};
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
