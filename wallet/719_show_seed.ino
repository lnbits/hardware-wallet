/**
   @brief Show the seed.
   While in interactive mode the client selects which word is shown, but the
   word itself remains on the hardware display and is never returned over the
   serial connection.
   In command-file mode all 24 words are reviewed on the hardware display and
   nothing is written to commands.out.txt.
   @param positionStr: String (optional). Position of the word to be displayed.
   @return CommandResponse
    - the seed word at the specified position to the hardware display
    - `/seed {position} displayed` to acknowledge on-device display.
*/
CommandResponse executeShowSeed(String positionStr) {
  if (global.authenticated == false) {
    return {"Enter password!", "8 numbers/letters"};
  }

  return showSeed(positionStr);
}

CommandResponse showSeed(String positionStr) {
  if (global.hasCommandsFile == true) {
    reviewSeedOnDevice(global.mnemonic);
    return {"Show Seed", "Done"};
  }
  return showSeedWordAtPosition(positionStr);
}

void reviewSeedOnDevice(const String &mnemonic) {
  int position = 1;
  while (true) {
    showDiceMnemonicWord(
      position,
      getWordAtPosition(mnemonic, position - 1)
    );

    bool moveForward = awaitPhysicalReviewApproval();
    if (moveForward && position == 24) return;
    if (moveForward) {
      position++;
    } else if (position > 1) {
      position--;
    }
  }
}

CommandResponse showSeedWordAtPosition(String positionStr) {
  int position = positionStr.toInt();
  if (!positionStr || position == 0) {
    positionStr = "1";
    position = 1;
  }
  if (position > 24) {
    return {"Show Seed", "Done"};
  }

  String word = getWordAtPosition(global.mnemonic, position - 1);
  printMnemonicWord(positionStr, word);
  sendCommandOutput(COMMAND_SEED, positionStr + " displayed");
  word = "";

  armInputForPrompt();
  EventData event = awaitEvent();
  if (event.type == EVENT_BUTTON_ACTION) {
    event = handleButtonDownEvent(event, position);
  }
  return {"", "", 0, event};
}


EventData handleButtonDownEvent(EventData buttonEvent, int position) {
  if (buttonEvent.data == "cancel") {
    int previous = position > 1 ? position - 1 : 1;
    return {EVENT_INTERNAL_COMMAND, COMMAND_SEED + " " + String(previous)};
  }
  return {EVENT_INTERNAL_COMMAND, COMMAND_SEED + " " + String(position + 1)};
}
