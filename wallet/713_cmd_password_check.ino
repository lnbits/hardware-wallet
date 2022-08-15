CommandResponse executePasswordCheck(String commandData) {
  if (commandData == "") {
    return { "Enter password",  "8 numbers/letters", 0 };
  }
  String hash = hashPassword(commandData);
  if (g.passwordHash == hash) {
    g.authenticated = true;
    serialSendCommand(COMMAND_PASSWORD, String(g.authenticated));
    return {"Password correct!",   "Ready to sign sir!" };
  }
  g.authenticated = false;
  serialSendCommand(COMMAND_PASSWORD, String(g.authenticated));
  return {"Wrong password, try again", "8 numbers/letters"};
}
