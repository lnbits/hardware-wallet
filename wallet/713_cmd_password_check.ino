CommandResponse executePasswordCheck(String commandData) {
  if (commandData == "") {
    return { "Enter password",  "8 numbers/letters"};
  }
  String hash = hashStringData(commandData);
  if (global.passwordHash == hash) {
    global.authenticated = true;
    serialSendCommand(COMMAND_PASSWORD, String(global.authenticated));
    return {"Password correct!",   "Ready to sign sir!" };
  }
  global.authenticated = false;
  serialSendCommand(COMMAND_PASSWORD, String(global.authenticated));
  return {"Wrong password, try again", "8 numbers/letters"};
}
