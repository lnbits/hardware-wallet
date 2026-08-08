/**
   @brief Show the seed.
   While in interactive mode the client selects which word is shown, but the
   word itself remains on the hardware display and is never returned over the
   serial connection.
   In the "command file" mode all the seed words will be returned.
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
    return sendSeedToFile();
  }
  return showSeedWordAtPosition(positionStr);
}

CommandResponse sendSeedToFile() {
  commandOutToFile("Seed: " + global.mnemonic);
  return {"Show Seed", "Done"};
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

  EventData event = awaitEvent();
  if (event.type == EVENT_BUTTON_ACTION) {
    event = handleButtonDownEvent(event, position);
  }
  return {"", "", 0, event};
}


EventData handleButtonDownEvent(EventData buttonEvent, int position) {
  String buttonState = getWordAtPosition(buttonEvent.data, 1);
  if (buttonState == "1") {
    // on button up, just show the word again
    return {EVENT_INTERNAL_COMMAND, COMMAND_SEED + " " + String(position)};
  }

  String buttonNumber = getWordAtPosition(buttonEvent.data, 0);
  if (buttonNumber == "1")
    return {EVENT_INTERNAL_COMMAND, COMMAND_SEED + " " + String(position - 1)};
  return {EVENT_INTERNAL_COMMAND, COMMAND_SEED + " " + String(position + 1) };
}
