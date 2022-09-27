/**
 * @brief Check if the provided password is correct.
 * Set the global `authenticated` flag to true if password is correct, or to false otherwhise.
 * Set the global `passphrase` value if provided.
 * @param commandData - a string containing the password and optionaly the passphrase.
 * @return CommandResponse 
 */
CommandResponse executePasswordCheck(String commandData) {
  if (isEmptyParam(commandData)) {
    return { "Enter password",  "8 numbers/letters"};
  }
  String password = getWordAtPosition(commandData, 0);

  String hash = hashStringData(password);
  if (global.passwordHash == hash) {
    global.authenticated = true;
    global.passphrase = getWordAtPosition(commandData, 1);;
    serialSendCommand(COMMAND_PASSWORD, String(global.authenticated));
    return {"Password correct!",   "Ready to sign sir!" };
  }
  global.authenticated = false;
  serialSendCommand(COMMAND_PASSWORD, String(global.authenticated));
  return {"Wrong password, try again", "8 numbers/letters"};
}
