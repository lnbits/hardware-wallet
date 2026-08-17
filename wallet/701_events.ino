unsigned long lastTickTime = 0;
int counter = 10;

int lineNumber = 0;



EventData awaitEvent() {
  if (global.hasCommandsFile == true) {
    return awaitFileEvent();
  }
  return awaitSerialEvent();
}

EventData awaitFileEvent() {
  String line = getNextFileCommand();
  delay(500);
  return { EVENT_INTERNAL_COMMAND, line };
}

/**
   @brief Get the next command from file.
   It will enter an infinite loop when it reaches the end of the file.
   It skips empty lines and commented lines (that start with `#`).
   @return String
*/
String getNextFileCommand() {
  String line = "";
  do {
    line = getLineAtPosition(global.commands, lineNumber);
    lineNumber++;
    // Accept command files written with either Unix (LF) or Windows (CRLF)
    // line endings, and ignore whitespace-only lines.
    line.trim();
  } while (line.startsWith("#") || line == "");
  return line;
}

EventData awaitSerialEvent() {
  unsigned long  waitTime = millis();
  bool idle = true;
  while (true) {
    while (Serial.available() == 0) {
      // check if ok for pairing or if idle
      if (idle == true) {
        if  ((millis() - waitTime) > 60 * 1000) {
          idle = false;
          logo(0);
        } else if  (counter > 0 && ((millis() - lastTickTime) > 1000)) {
          counter--;
          lastTickTime = millis();
          logo(counter);
          if (counter == 0) counter = -1;
        }
      }

      EventData buttonEvent = checkButtonsState();
      if (buttonEvent.type == EVENT_BUTTON_ACTION) return buttonEvent;

      delay(5);

    }
    counter = -1;
    String data = Serial.readStringUntil('\n');
    data.trim();
    if (data != "") return { EVENT_SERIAL_DATA, data };
  }
}

EventData checkButtonsState() {
  char key = pollInputKey();
  if (key == '#') return {EVENT_BUTTON_ACTION, "confirm"};
  if (key == '*') return {EVENT_BUTTON_ACTION, "cancel"};
  return {"", ""};
}
