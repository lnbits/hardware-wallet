/**
   @brief Log out. The client will no longer be authenticated.
   A new `Log In` is required in order to execute secured commands.

   @param commandData: String. Not used.
   @return CommandResponse
   - authentication status to the UI
   - `/password-clear 1` to the client.
*/
CommandResponse executePasswordClear(String commandData) {
  global.authenticated = false;
  global.passphrase = "";

  serialSendCommand(COMMAND_PASSWORD_CLEAR, "1");
  showMessage("Logging out...", "");
  delay(2000);

  return {"Logged out",  "Enter password"};
}
