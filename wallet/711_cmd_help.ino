/**
   @brief Show info about the existing commands.

   @param commandData: String.  Not used.
   @return CommandResponse
*/
CommandResponse executeHelp(String commandData) {
  help();
  return {"More info at:", "github.com/lnbits/hardware-wallet"};
}

void help()
{
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(0, 0);
  tft.println("Commands");
  tft.setCursor(0, 20);
  tft.setTextSize(1);

  tft.println("'/help' show available commands");
  tft.println("'/restore <BIP39 seed words seperated by space>' will restore from seed");
  tft.println("'/wipe' will completely wipe device and creat a new wallet");
  tft.println("'/password' login by providing a password");
  tft.println("'/password-clear' logout");
  tft.println("'/seed' will show the seed on the hww display");
  tft.println("'/psbt' will parse valid psbt and show its outputs and fee");
  tft.println("'/sign' will sign valid psbt");
  tft.println("'/xpub' will return the XPub for a provided derivation path");

  logInfo("Commands");
  logInfo("'/help' show available commands");
  logInfo("'/restore <BIP39 seed words seperated by space>' will restore from seed");
  logInfo("'/wipe' will completely wipe device and creat a new wallet");
  logInfo("'/password' login by providing a password");
  logInfo("'/password-clear' logout");
  logInfo("'/seed' will show the seed on the hww display");
  logInfo("'/psbt' will parse valid psbt and show its outputs and fee");
  logInfo("'/sign' will sign valid psbt");
  logInfo("'/xpub' will return the XPub for a provided derivation path");
  delay(10000);
}
