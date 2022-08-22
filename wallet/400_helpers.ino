//========================================================================//
//================================HELPERS=================================//
//========================================================================//

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
  int i = 0;
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

SeedData parseSeedData(String mnemonicWithPassphrase) {
  if (!mnemonicWithPassphrase) {
    return {"", ""};
  }
  int separatorPos = mnemonicWithPassphrase.indexOf("/");
  if (separatorPos == -1) {
    return {mnemonicWithPassphrase, ""};
  }
  String mnemonic = mnemonicWithPassphrase.substring(0, separatorPos);
  String passphrase = mnemonicWithPassphrase.substring(separatorPos + 1, mnemonicWithPassphrase.length());
  return {mnemonic, passphrase};
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
