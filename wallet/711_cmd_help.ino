CommandResponse executeHelp(String commandData) {
  // help();
  setupSD();
  const String testFileName = "/abc.txt";
  const String testFileNameOut = "/abc-out-8.txt";
  FileData fd = readFile(SD, testFileName.c_str());
  logSerial("fd success: " + String(fd.success));
  logSerial("fd data: " + fd.data);
  String line = "";
  writeFile(SD, testFileNameOut.c_str(), "IN " + testFileNameOut + "\n");
  int lineNumber = 0;
  do {
    logSerial("line-number[" + String(lineNumber) + "]:");
    line = getLineAtPosition(fd.data, lineNumber);
    const String l2 = String("["+String(lineNumber) + "]: " + line);
    line.toLowerCase();
    appendFile(SD, testFileNameOut.c_str(), line.c_str());
    lineNumber++;
  } while (line != ""); // last line not read!

  logSerial("line-last[" + String(lineNumber) + "]: " + line);
  // appendFile(SD, testFileNameOut.c_str(), "OUT".c_str());

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

  logSerial("Commands");
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
