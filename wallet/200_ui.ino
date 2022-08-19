//========================================================================//
//===============================UI STUFF=================================//
//========================================================================//

void logo(int counter) {
  String title = "Signer";
  String subTitle = "LNbits/ubitcoin HWW";
  if (counter > 0) {
    title = title + " " + String(counter);
    subTitle = "Open for pairing";
  }
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(4);
  tft.setCursor(0, 30);
  tft.print(title);
  tft.setTextSize(2);
  tft.setCursor(0, 80);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.print(subTitle);
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
