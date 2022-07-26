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
    logInfo("Failed psbt: " + psbtBase64);
    sendCommandOutput(COMMAND_SEND_PSBT, "psbt_parse_failed");
    return {"Failed parsing",  "Send PSBT again"};
  }

  HDPrivateKey hd(global.mnemonic, global.passphrase, network);
  // check if it is valid
  if (!hd) {
    sendCommandOutput(COMMAND_SEND_PSBT, "invalid_mnemonic'");
    return {"Invalid Mnemonic", ""};
  }

  sendCommandOutput(COMMAND_SEND_PSBT, "1");

  for (int i = 0; i < psbt.tx.outputsNumber; i++) {
    CommandResponse outRes = confirmOutputDetails(psbt, hd, i, network);
    if (outRes.message != "") return outRes;
  }

  confirmFeeDetails(psbt.fee());

  return signPsbt(psbt, hd);

}


CommandResponse confirmOutputDetails(PSBT psbt, HDPrivateKey hd, int index, const Network * network) {
  if (global.hasCommandsFile == true) {
    return writeOutputToFile(psbt, hd, index, network);
  }
  return showOutputForConfirmation(psbt, hd, index, network);
}

CommandResponse showOutputForConfirmation(PSBT psbt, HDPrivateKey hd, int index, const Network * network) {
  printOutputDetails(psbt, hd, index, network);

  EventData event = awaitEvent();
  if (event.type != EVENT_SERIAL_DATA) {
    return {"Operation Canceled", "button pressed" };
  };
  String data = event.data;

  Command c = decryptAndExtractCommand(data);
  if (c.cmd == COMMAND_CANCEL) {
    return {"Operation Canceled", "`/help` for details" };
  }
  if (c.cmd != COMMAND_CONFIRM_NEXT) {
    return executeUnknown("Expected: " + COMMAND_CONFIRM_NEXT);
  }
  return {"", ""};
}

CommandResponse writeOutputToFile(PSBT psbt, HDPrivateKey hd, int index, const Network * network) {
  String sats = int64ToString(psbt.tx.txOuts[index].amount);
  String output = "Output " + String(index) + "\n" +
                  "Address: " + psbt.tx.txOuts[index].address(network) + "\n" +
                  "Amount: " + sats + " sat";

  commandOutToFile(output);
  return {"", ""};
}

void confirmFeeDetails(uint64_t fee) {
  if (global.hasCommandsFile == true) {
    writeFeeToFile(fee);
  } else {
    printFeeDetails(fee);
  }

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
  showMessage("Please wait", "Signing PSBT...");
  delay(500);

  uint8_t signedInputCount = psbt.sign(hd);
  sendCommandOutput(COMMAND_SIGN_PSBT,  String(signedInputCount) + " " + psbt.toBase64());

  return { "Signed inputs:", String(signedInputCount) };
}

CommandResponse confirmAndSignPsbt(PSBT psbt, HDPrivateKey hd) {
  EventData event = awaitEvent();
  if (event.type != EVENT_SERIAL_DATA) {
    return {"Operation Canceled", "`/help` for details" };
  }
  String data = event.data;

  Command c = decryptAndExtractCommand(data);
  if (c.cmd == COMMAND_SIGN_PSBT) {
    showMessage("Confirm", "Press button to confirm");
    EventData event = awaitEvent();
    if (event.type != EVENT_BUTTON_ACTION) {
      return {"Operation Canceled", "" };
    };

    showMessage("Please wait", "Signing PSBT...");

    uint8_t signedInputCount = psbt.sign(hd);

    sendCommandOutput(COMMAND_SIGN_PSBT,  String(signedInputCount) + " " + psbt.toBase64());
    return { "Signed inputs:", String(signedInputCount) };
  }
  if (c.cmd = COMMAND_CANCEL) {
    return { "Operation Canceled",  "`/help` for details" };
  }
  return  executeUnknown("Expected: " + COMMAND_SIGN_PSBT);
}
