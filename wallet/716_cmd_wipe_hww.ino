/**
   @brief Clear the current seed and password.
   Generate a new `seed` and initialize the `password` with a new value.

   @param password: String. Minimum 8 character string.
   @return CommandResponse
      - wipe status to the UI.
      - `/wipe {status}` to the client. Status is `1` for success and `0` for failure.
*/
CommandResponse executeWhipeHww(String password) {
  if (isEmptyParam(password)) {
    return { "Enter new password", "8 numbers/letters"};
  }

  showMessage("Resetting...", "");
  delay(2000);

  HwwInitData data = initHww(password, "", "", true);
  delay(DELAY_MS);
  global.authenticated = data.success;

  sendCommandOutput(COMMAND_WIPE,  String(global.authenticated));

  if (global.authenticated == true) {
    global.passwordHash = data.passwordHash;
    global.mnemonic = data.mnemonic;
    global.passphrase = "";
    return { "Successfully wiped!",  "Every new beginning comes from some other beginning's end."};
  }
  return {"Error, try again"  "8 numbers/letters"};
}
