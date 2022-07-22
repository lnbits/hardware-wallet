//========================================================================//
//===============================UI STUFF=================================//
//========================================================================//

void logo() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(4);
  tft.setCursor(0, 30);
  tft.print("BabyBowser");
  tft.setTextSize(2);
  tft.setCursor(0, 80);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.print("LNbits/ubitcoin HWW");
  Serial.println("BabyBowser LNbits/ubitcoin HWW");
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
  Serial.println(message);
  Serial.println(additional);
}

void help()
{
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(0, 0);
  tft.println("BabyBowser Commands");
  tft.setCursor(0, 20);
  tft.setTextSize(1);
  tft.println("'wipe' will completely wipe device and creat a new wallet");
  tft.println("'restore <BIP39 seed words seperated by space>' will restore from seed");
  tft.println("'seed' will send seed to serial output");
  tft.println("'cHNid...' Will parse valid psbt");
  tft.println("'sign' Will sign valid psbt");
  Serial.println("BabyBowser Commands");
  Serial.println("'wipe' will completely wipe device and creat a new wallet");
  Serial.println("'restore <BIP39 seed words seperated by space>' will restore from seed");
  Serial.println("'seed' will send seed to serial output");
  Serial.println("'cHNid...' Will parse valid psbt");
  Serial.println("'sign' Will sign valid psbt");
  delay(10000);
}

void printMnemonic(String mn) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(0, 0);
  tft.println("Word list:");
  tft.setCursor(0, 20);
  tft.setTextSize(1);
  tft.println(mn);
}
