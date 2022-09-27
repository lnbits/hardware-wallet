unsigned long lastTickTime = 0;
int counter = 10;

int button1State = HIGH;
int button2State = HIGH;
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
  } while (line.startsWith("#") || line == "");
  return line;
}

EventData awaitSerialEvent() {
  unsigned long  waitTime = millis();
  bool idle = true;
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
      } else if (counter == 0) {
        logo(counter);
        counter--;
      }
    }

    EventData buttonEvent = checkButtonsState();
    if (buttonEvent.type == EVENT_BUTTON_ACTION) return buttonEvent;

  }
  counter = -1;
  String data = Serial.readStringUntil('\n');
  return { EVENT_SERIAL_DATA, data };
}

EventData checkButtonsState() {
  int button1NewState = digitalRead(global.button1Pin);
  if (button1NewState != button1State) {
    logInfo("button 1: " + String(button1NewState));
    button1State = button1NewState;
    return { EVENT_BUTTON_ACTION, "1 " + String(button1NewState)};
  }

  int button2NewState = digitalRead(global.button2Pin);
  if (button2NewState != button2State) {
    logInfo("button 2: " + String(button2NewState));
    button2State = button2NewState;
    return { EVENT_BUTTON_ACTION, "2 " + String(button2NewState)};
  }
  return {"", ""};
}
