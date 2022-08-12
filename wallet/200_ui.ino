//========================================================================//
//===============================UI STUFF=================================//
//========================================================================//

void logo() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(4);
  tft.setCursor(0, 30);
  tft.print("Signer");
  tft.setTextSize(2);
  tft.setCursor(0, 80);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.print("LNbits/ubitcoin HWW");
  logSerial("HardwareSigner LNbits/ubitcoin");
}

void showMessage(String message, String additional)
{
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(0, 30);
  tft.println(message);
  tft.setCursor(0, 80);
  tft.setTextSize(2);
  tft.println(additional);
  logSerial(message);
  logSerial(additional);
}

void help()
{
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(0, 0);
  tft.println("HardwareSigner Commands");
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

  logSerial("HardwareSigner Commands");
  logSerial("'/help' show available commands");
  logSerial("'/restore <BIP39 seed words seperated by space>' will restore from seed");
  logSerial("'/wipe' will completely wipe device and creat a new wallet");
  logSerial("'/password' login by providing a password");
  logSerial("'/password-clear' logout");
  logSerial("'/seed' will show the seed on the hww display");
  logSerial("'/psbt' will parse valid psbt and show its outputs and fee");
  logSerial("'/sign' will sign valid psbt");
  logSerial("'/xpub' will return the XPub for a provided derivation path");
  delay(10000);
}

void printMnemonicWord(String position, String word) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(0, 10);
  tft.println("Word " + position + ":");
  tft.println("");
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(3);
  tft.println(word);
}
