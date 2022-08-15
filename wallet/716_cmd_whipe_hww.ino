CommandResponse executeinitHww(String password) {
  if (password == "") {
    return { "Enter new password", "8 numbers/letters"};
  }

  showMessage("Resetting...", "");
  delay(2000);

  g.authenticated = initHww(password, "");
  serialSendCommand(COMMAND_WIPE,  String(g.authenticated));
  if (g.authenticated == true) {
    return { "Successfully wiped!",  "Every new beginning comes from some other beginning's end."};
  }
  return {"Error, try again"  "8 numbers/letters"};
}