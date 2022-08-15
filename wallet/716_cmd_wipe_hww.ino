CommandResponse executeWhipeHww(String password) {
  if (password == "") {
    return { "Enter new password", "8 numbers/letters"};
  }

  showMessage("Resetting...", "");
  delay(2000);

  global.authenticated = initHww(password, "");
  serialSendCommand(COMMAND_WIPE,  String(global.authenticated));
  if (global.authenticated == true) {
    return { "Successfully wiped!",  "Every new beginning comes from some other beginning's end."};
  }
  return {"Error, try again"  "8 numbers/letters"};
}
