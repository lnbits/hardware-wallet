CommandResponse executePasswordClear(String commandData) {
  g.authenticated = false;
  serialSendCommand(COMMAND_PASSWORD_CLEAR, "1");
  showMessage("Logging out...", "");
  delay(2000);

  return {"Logged out",  "Enter password"};
}