/**
   @brief Show info about the existing commands.

   @param commandData: String.  Not used.
   @return CommandResponse
*/
CommandResponse executeHelp(String commandData) {
  help();
  return {"More info at:", "github.com/lnbits/hardware-wallet"};
}

void printHelpLine(const String &line) {
  tft.setTextSize(uiFittedTextSize(line, 1, tft.width()));
  tft.println(line);
}

void help()
{
  beginUiScreen();
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(uiFittedTextSize("Commands", 2, tft.width()));
  tft.setCursor(0, 0);
  tft.println("Commands");
  tft.setCursor(0, 8 * uiFittedTextSize("Commands", 2, tft.width()) + 4);

  printHelpLine("'/help' show available commands");
  printHelpLine("'/restore <BIP39 seed words seperated by space>' will restore from seed");
  printHelpLine("'/create <password>' creates a 24-word seed from 100 dice rolls");
  printHelpLine("'/wipe' will completely wipe device and creat a new wallet");
  printHelpLine("'/password' login by providing a password");
  printHelpLine("'/password-clear' logout");
  printHelpLine("'/seed' will show the seed on the hww display");
  printHelpLine("'/trng' shows a live 1-100 RNG distribution");
  printHelpLine("'/psbt' will parse valid psbt and show its outputs and fee");
  printHelpLine("'/sign' will sign valid psbt");
  printHelpLine("'/xpub' defaults to Mainnet BIP84 account");

  logInfo("Commands");
  logInfo("'/help' show available commands");
  logInfo("'/restore <BIP39 seed words seperated by space>' will restore from seed");
  logInfo("'/create <password>' creates a 24-word seed from 100 dice rolls");
  logInfo("'/wipe' will completely wipe device and creat a new wallet");
  logInfo("'/password' login by providing a password");
  logInfo("'/password-clear' logout");
  logInfo("'/seed' will show the seed on the hww display");
  logInfo("'/trng' shows a live 1-100 RNG distribution");
  logInfo("'/psbt' will parse valid psbt and show its outputs and fee");
  logInfo("'/sign' will sign valid psbt");
  logInfo("'/xpub' defaults to Mainnet m/84'/0'/0'");
  delay(10000);
}
