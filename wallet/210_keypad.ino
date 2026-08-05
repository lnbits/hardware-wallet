//========================================================================//
//=============================4x3 KEYPAD=================================//
//========================================================================//

const char KEYPAD_KEYS[4][3] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};

void setupKeypad() {
  for (int row = 0; row < 4; row++) {
    pinMode(KEYPAD_ROW_PINS[row], INPUT_PULLUP);
  }
  for (int column = 0; column < 3; column++) {
    pinMode(KEYPAD_COLUMN_PINS[column], INPUT);
  }
}

char scanKeypad() {
  for (int column = 0; column < 3; column++) {
    pinMode(KEYPAD_COLUMN_PINS[column], OUTPUT);
    digitalWrite(KEYPAD_COLUMN_PINS[column], LOW);
    delayMicroseconds(5);

    for (int row = 0; row < 4; row++) {
      if (digitalRead(KEYPAD_ROW_PINS[row]) == LOW) {
        pinMode(KEYPAD_COLUMN_PINS[column], INPUT);
        return KEYPAD_KEYS[row][column];
      }
    }
    pinMode(KEYPAD_COLUMN_PINS[column], INPUT);
  }
  return 0;
}

char waitForKeypadKey() {
  while (true) {
    char key = 0;
    while (key == 0) {
      key = scanKeypad();
      delay(10);
    }

    // Debounce the press, then wait for release so a held key is entered once.
    delay(25);
    if (scanKeypad() != key) {
      continue;
    }
    while (scanKeypad() != 0) {
      delay(10);
    }
    delay(25);
    return key;
  }
}
