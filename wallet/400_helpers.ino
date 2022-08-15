//========================================================================//
//================================HELPERS=================================//
//========================================================================//

unsigned long startTime = 0;
bool idle = false;

String awaitSerialData() {
  startTime = millis();
  idle = true;
  while (Serial.available() == 0) {
    // just loop and wait
    if (idle == true && (millis() - startTime) > 60 * 1000) {
      idle = false;
      logo();
    }
  }
  return Serial.readStringUntil('\n');
}

HwwInitData initHww(String password, String mnemonic) {
  if (isAlphaNumeric(password) == false)
    return {"", "", false};

  deleteFile(SPIFFS, "/mn.txt");
  deleteFile(SPIFFS, "/hash.txt");
  if (mnemonic == "") {
    mnemonic = createMnemonic(24); // todo: allow 12 also
  }
  
  String passwordHash  = hashPassword(password);
  writeFile(SPIFFS, "/hash.txt", passwordHash);  
  writeFile(SPIFFS, "/mn.txt", mnemonic);

  return {passwordHash, mnemonic, true};
}

int getMnemonicBytes(String menmonicSentence) {
  int wc = wordCount(menmonicSentence);
  switch (wc)
  {
    case 12:
      return 16;
    case 15:
      return 20;
    case 18:
      return 24;
    case 21:
      return 28;
    case 24:
      return 32;
    default:
      return 0;
  }
}

int wordCount(String s) {
  int count = 1;
  for (int i = 0; i < s.length(); i++)
    if (s[i] == ' ') count++;
  return count;
}


bool isAlphaNumeric(String instr) {
  if (instr.length() < 8) {
    return false;
  }
  for (int i = 0; i < instr.length(); i++) {
    if (isalpha(instr[i])) {
      continue;
    }
    if (isdigit(instr[i])) {
      continue;
    }
    return false;
  }
  return true;
}

String int64ToString(uint64_t value) {
  String s = "";
  while (value != 0) {
    s = String((int)(value % 10)) + s;
    value = (uint64_t) value / 10;
  }
  return s;
}

String getWordAtPosition(String str, int position) {
  String s = str.substring(0);
  int spacePos = 0;
  int i = 1;
  while (spacePos != -1) {
    spacePos = s.indexOf(" ");
    if (i == position) {
      if (spacePos == -1) return s;
      return s.substring(0, spacePos);
    }
    s = s.substring(spacePos + 1);
    i++;
  }

  return "";
}

Command extractCommand(String s) {
  int spacePos = s.indexOf(" ");
  String command = s.substring(0, spacePos);
  if (spacePos == -1) {
    return {command, ""};
  }
  String commandData = s.substring(spacePos + 1, s.length());
  return {command, commandData};
}