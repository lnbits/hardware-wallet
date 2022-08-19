CommandResponse executeShowSeed(String position) {
  if (global.authenticated == false) {
    return {"Enter password!", "8 numbers/letters"};
  }
  if (!position || position.toInt() == 0) {
    return {"Bad word position",  "`/help` for details " };
  }
  String word = getWordAtPosition(global.encrytptedMnemonic, position.toInt() - 1);
  printMnemonicWord(position, word);
  return {"", ""};
}