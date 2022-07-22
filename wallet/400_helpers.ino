//========================================================================//
//================================HELPERS=================================//
//========================================================================//


String awaitSerialData() {
  while (Serial.available() == 0) {
    // just loop and wait
  }
  return Serial.readStringUntil('\n');
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