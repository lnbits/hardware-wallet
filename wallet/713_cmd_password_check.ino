/**
   @brief Check if the provided password is correct.
   Set the global `authenticated` flag to true if password is correct, or to false otherwhise.
   Set the global `passphrase` value if provided.
   @param data: String. Space separated values. Use minus (`-`) to skip the value.
    Value significance by position:
    0 - password: String. Minimum 8 characters.
    1 - passphease: String (optional). The BIP39 passphrase.
   @return CommandResponse
    - authentication status to the UI
    - `/password {status}` to the client. Status is `1` for success and `0` for `failure` to authenticate.
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
    sendCommandOutput(COMMAND_PASSWORD, String(global.authenticated));
    return {"Password correct!",   "Ready to sign sir!" };
  }
  global.authenticated = false;
  sendCommandOutput(COMMAND_PASSWORD, String(global.authenticated));
  return {"Wrong password, try again", "8 numbers/letters"};
}
