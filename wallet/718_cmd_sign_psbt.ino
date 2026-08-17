/**
   @brief Sign a PSBT.

   @param commandData: String. Space separated values. Use minus (`-`) to skip the value.
    Value significance by position:
    0 - networkName: String. Can be `Testnet` or `Mainnet`.
    1 - psbtBase64: String. Base64 encoded PSBT
   @return CommandResponse
    - sign status and the number of outputs to the UI.
    - `/psbt psbt_parse_failed` to the client if the PSBT cannot be parsed.
    - `/psbt invalid_mnemonic`  to the client if the stored mnemonic is invalid.
    - `psbt 1` to the client  if the PSBT can be signed
       - `sign {inputCount} {base64Psbt}` to the client.
          - `inputCount` is the numner of signed inputs. Not necessarlily all inputs are signed by this device.
          - `base64Psbt` the signed psbt in base64 format.
*/
CommandResponse executeSignPsbt(String commandData) {
  if (global.authenticated == false) {
    return { "Enter password!", "8 numbers/letters"};
  }

  showMessage("Please wait", "Parsing PSBT...");
  // todo: use word at position
  int spacePos = commandData.indexOf(" ");
  String networkName = commandData.substring(0, spacePos);
  String psbtBase64 = commandData.substring(spacePos + 1, commandData.length() );

  const Network * network;
  if (networkName == "Mainnet") {
    network = &Mainnet;
  } else if (networkName == "Testnet") {
    network = &Testnet;
  } else {
    return { "Unknown Network", "Must be Mainent or Testnet"};
  }

  PSBT psbt = parseBase64Psbt(psbtBase64);
  if (!psbt) {
    logInfo("Failed to parse PSBT");
    sendCommandOutput(COMMAND_SEND_PSBT, "psbt_parse_failed");
    return {"Failed parsing",  "Send PSBT again"};
  }
  HDPrivateKey hd(global.mnemonic, global.passphrase, network);
  // check if it is valid
  if (!hd) {
    sendCommandOutput(COMMAND_SEND_PSBT, "invalid_mnemonic");
    return {"Invalid Mnemonic", ""};
  }

  // SD signing is a single command, so preserve its acknowledgement before the
  // on-device review. WebSerial keeps the /psbt request pending until every
  // output and the fee have been physically reviewed. This prevents the host
  // from treating a parse acknowledgement as approval to skip trusted-display
  // review.
  if (global.hasCommandsFile == true) {
    sendCommandOutput(COMMAND_SEND_PSBT, "1");
  }

  for (int i = 0; i < psbt.tx.outputsNumber; i++) {
    CommandResponse outRes = confirmOutputDetails(psbt, hd, i, network);
    if (outRes.message != "") return outRes;
  }

  CommandResponse feeRes = confirmFeeDetails(psbt.fee());
  if (feeRes.message != "") return feeRes;

  if (global.hasCommandsFile == false) {
    sendCommandOutput(COMMAND_SEND_PSBT, "1");
  }

  return signPsbt(psbt, hd);

}


CommandResponse confirmOutputDetails(PSBT psbt, HDPrivateKey hd, int index, const Network * network) {
  if (global.hasCommandsFile == true) {
    writeOutputToFile(psbt, index, network);
  }
  printOutputDetails(psbt, hd, index, network);
  printPhysicalReviewControls();
  if (awaitPhysicalReviewApproval() == false) {
    sendCommandOutput(COMMAND_SEND_PSBT, "review_rejected");
    return {"Operation canceled", "Output rejected"};
  }
  return {"", ""};
}

void writeOutputToFile(PSBT psbt, int index, const Network * network) {
  String sats = int64ToString(psbt.tx.txOuts[index].amount);
  String output = "Output " + String(index) + "\n" +
                  "Address: " + psbt.tx.txOuts[index].address(network) + "\n" +
                  "Amount: " + sats + " sat";

  commandOutToFile(output);
}

CommandResponse confirmFeeDetails(uint64_t fee) {
  if (global.hasCommandsFile == true) {
    writeFeeToFile(fee);
  }
  printFeeDetails(fee);
  printPhysicalReviewControls();
  if (awaitPhysicalReviewApproval() == false) {
    sendCommandOutput(COMMAND_SEND_PSBT, "review_rejected");
    return {"Operation canceled", "Fee rejected"};
  }
  return {"", ""};
}

void writeFeeToFile(uint64_t fee) {
  String sats = int64ToString(fee);
  commandOutToFile("Fee: " + sats + " sat");
}

CommandResponse signPsbt(PSBT psbt, HDPrivateKey hd) {
  if (global.hasCommandsFile == true) {
    return signPsbtToFile(psbt, hd);
  }
  return confirmAndSignPsbt(psbt, hd);
}

CommandResponse signPsbtToFile(PSBT psbt, HDPrivateKey hd) {
  showConfirmCancelMessage("Sign reviewed PSBT?", "Review complete");
  if (awaitPhysicalReviewApproval() == false) {
    sendCommandOutput(COMMAND_SEND_PSBT, "review_rejected");
    return {"Operation canceled", "Signing rejected"};
  }

  showMessage("Please wait", "Signing PSBT...");
  delay(500);

  uint8_t signedInputCount = psbt.sign(hd);
  sendCommandOutput(COMMAND_SIGN_PSBT,  String(signedInputCount) + " " + psbt.toBase64());

  return { "Signed inputs:", String(signedInputCount) };
}

CommandResponse confirmAndSignPsbt(PSBT psbt, HDPrivateKey hd) {
  while (true) {
    EventData event = awaitEvent();
    if (event.type != EVENT_SERIAL_DATA) continue;

    Command c = decryptAndExtractCommand(event.data);
    // Compatibility with older web clients. The command no longer advances
    // trusted-display review and is ignored until the client asks to sign.
    if (c.cmd == COMMAND_CONFIRM_NEXT) continue;
    if (c.cmd == COMMAND_CANCEL) {
      return {"Operation Canceled", "`/help` for details" };
    }
    if (c.cmd != COMMAND_SIGN_PSBT) {
      return executeUnknown("Expected: " + COMMAND_SIGN_PSBT);
    }

    showConfirmCancelMessage("Confirm signing", "Outputs and fee reviewed");
    if (awaitPhysicalReviewApproval() == false) {
      sendCommandOutput(COMMAND_SIGN_PSBT, "review_rejected");
      return {"Operation Canceled", "Signing rejected" };
    }

    showMessage("Please wait", "Signing PSBT...");

    uint8_t signedInputCount = psbt.sign(hd);

    sendCommandOutput(COMMAND_SIGN_PSBT,  String(signedInputCount) + " " + psbt.toBase64());
    return { "Signed inputs:", String(signedInputCount) };
  }
}
