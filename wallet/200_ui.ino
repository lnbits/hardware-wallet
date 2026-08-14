//========================================================================//
//===============================UI STUFF=================================//
//========================================================================//

void logo(int counter) {
  String title = "Bowser HWW";
  String subTitle = "ubitcoin powered signer";
  if (counter > 0) {
    title += " " + String(counter);
    subTitle = "Open for pairing";
  }

  beginUiScreen();
  int16_t contentHeight = uiContentHeight();
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(uiFittedTextSize(title, 3, tft.width() - 8));
  tft.setCursor(4, max(4, contentHeight / 5));
  tft.print(title);
  tft.setTextSize(uiFittedTextSize(
    subTitle,
    counter > 0 ? 2 : 1,
    tft.width() - 8
  ));
  tft.setCursor(4, max(24, contentHeight * 3 / 5));
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.print(subTitle);
  String version = "version: " + env.version + " / " + BOARD.id;
  tft.setTextSize(uiFittedTextSize(version, 1, tft.width() - 8));
  tft.setCursor(4, max(40, contentHeight - 20));
  tft.print(version);
}

void showBootLogo() {
  beginUiScreen();
  drawBowserLogo((uiContentHeight() - BOWSER_LOGO_HEIGHT) / 2);
}

void showFirmwareHash(String firmwareHash) {
  beginUiScreen(UiControls::ContinueOnly);
  int16_t contentHeight = uiContentHeight();
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  String heading = "Firmware v" + env.version + " SHA-256";
  uint8_t headingSize = uiFittedTextSize(heading, 1, tft.width() - 4);
  tft.setTextSize(headingSize);
  tft.setCursor(2, 2);
  tft.print(heading);

  if (firmwareHash.length() == 64) {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    uint8_t hashSize = uiFittedTextSize(
      firmwareHash.substring(0, 16),
      2,
      tft.width() - 4
    );
    tft.setTextSize(hashSize);
    tft.setCursor(2, 2 + uiTextPixelHeight(headingSize) + 2);
    for (int offset = 0; offset < 64; offset += 16) {
      tft.println(firmwareHash.substring(offset, offset + 16));
    }
  } else {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setTextSize(uiFittedTextSize("Hash unavailable", 2, tft.width() - 4));
    tft.setCursor(2, min(42, contentHeight / 3));
    tft.print("Hash unavailable");
  }

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(uiFittedTextSize("Compare with release", 1, tft.width() - 4));
  int16_t guidanceY = max(84, contentHeight - (BOARD.hasTouchscreen ? 28 : 44));
  tft.setCursor(2, guidanceY);
  tft.println("Compare with release");
  if (!BOARD.hasTouchscreen) {
    tft.setCursor(2, contentHeight - 12);
    if (BOARD.singleButtonLongPressCancels) {
      tft.print("Tap BOOT to continue");
    } else {
      tft.print("Press # / BTN1");
    }
  }
}

void drawBowserLogo(int y) {
  int x = (tft.width() - BOWSER_LOGO_WIDTH) / 2;
  bool previousSwapBytes = tft.getSwapBytes();
  tft.setSwapBytes(true);
  tft.pushImage(
    x,
    y,
    BOWSER_LOGO_WIDTH,
    BOWSER_LOGO_HEIGHT,
    BOWSER_LOGO_PIXELS
  );
  tft.setSwapBytes(previousSwapBytes);
}

void drawMessageContent(String message, String additional) {
  int16_t contentHeight = uiContentHeight();
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(uiFittedTextSize(message, 2, tft.width() - 8));
  tft.setCursor(4, max(6, contentHeight / 5));
  tft.println(message);
  tft.setCursor(4, max(34, contentHeight * 3 / 5));
  tft.setTextSize(uiFittedTextSize(additional, 2, tft.width() - 8));
  tft.println(additional);
  logInfo(message);
  logInfo(additional);
}

void showMessage(String message, String additional) {
  beginUiScreen();
  drawMessageContent(message, additional);
}

void showConfirmCancelMessage(String message, String additional) {
  beginUiScreen(UiControls::ConfirmCancel);
  drawMessageContent(message, additional);
}

void printMnemonicWord(String position, String word) {
  beginUiScreen(UiControls::PreviousNext);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  String heading = "Word " + position + ":";
  tft.setTextSize(uiFittedTextSize(heading, 2, tft.width() - 8));
  tft.setCursor(4, 8);
  tft.println(heading);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(uiFittedTextSize(word, 3, tft.width() - 8));
  int16_t wordY = uiContentHeight() / 2;
  tft.setCursor(4, wordY < 54 ? wordY : 54);
  tft.println(word);
}

void showDiceRollProgress(int rollCount, char latestRoll) {
  bool complete = rollCount == DICE_ROLL_COUNT;
  beginUiScreen(UiControls::DicePad, complete);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  int16_t y = 2;
  uint8_t textSize = uiFittedTextSize("Dice wallet", 2, tft.width() - 8);
  tft.setTextSize(textSize);
  tft.setCursor(4, y);
  tft.println("Dice wallet");
  y += uiTextPixelHeight(textSize) + 2;
  textSize = uiFittedTextSize("Enter rolls 1-6", 2, tft.width() - 8);
  tft.setTextSize(textSize);
  tft.setCursor(4, y);
  tft.println("Enter rolls 1-6");
  y += uiTextPixelHeight(textSize) + 2;
  String progress = String(rollCount) + "/" + String(DICE_ROLL_COUNT);
  textSize = uiFittedTextSize(progress, 2, tft.width() - 8);
  tft.setTextSize(textSize);
  tft.setCursor(4, y);
  tft.println(progress);

  y += uiTextPixelHeight(textSize) + 2;
  tft.setCursor(4, y);
  if (complete) {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setTextSize(uiFittedTextSize("Select Done", 2, tft.width() - 8));
    tft.println("Select Done");
  } else if (latestRoll != 0) {
    String latest = "Last roll: " + String(latestRoll);
    tft.setTextSize(uiFittedTextSize(latest, 2, tft.width() - 8));
    tft.println(latest);
  } else {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setTextSize(uiFittedTextSize("* removes last", 2, tft.width() - 8));
    tft.println("* removes last");
  }
}

void showDiceMnemonicWord(int position, String word) {
  beginUiScreen(UiControls::PreviousNext);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  String heading = "Write word " + String(position);
  tft.setTextSize(uiFittedTextSize(heading, 2, tft.width() - 8));
  tft.setCursor(4, 4);
  tft.println(heading);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(uiFittedTextSize(word, 3, tft.width() - 8));
  int16_t wordY = uiContentHeight() / 2;
  tft.setCursor(4, wordY < 50 ? wordY : 50);
  tft.println(word);
  if (!BOARD.hasTouchscreen) {
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    String guidance = position == 24 ? "* back    # finish" : "* back    # next";
    tft.setTextSize(uiFittedTextSize(guidance, 1, tft.width() - 8));
    tft.setCursor(4, uiContentHeight() - 12);
    tft.println(guidance);
  }
}

int qrVersionFromStringLength(int stringLength) {
  if (stringLength <= 17) return 1;
  if (stringLength <= 32) return 2;
  if (stringLength <= 53) return 3;
  if (stringLength <= 134) return 6;
  if (stringLength <= 367) return 11;
  return 28;
}

void showQRCode(String value) {
  int version = qrVersionFromStringLength(value.length());
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(version)];
  qrcode_initText(&qrcode, qrcodeData, version, 0, value.c_str());

  beginUiScreen(UiControls::ToggleOnly);
  const int quietZone = 2;
  int available = tft.width() < uiContentHeight() ? tft.width() : uiContentHeight();
  int pixelsPerModule = available / (qrcode.size + quietZone * 2);
  if (pixelsPerModule < 1) pixelsPerModule = 1;
  int qrPixels = (qrcode.size + quietZone * 2) * pixelsPerModule;
  int originX = (tft.width() - qrPixels) / 2 + quietZone * pixelsPerModule;
  int originY = (uiContentHeight() - qrPixels) / 2 + quietZone * pixelsPerModule;

  for (uint8_t y = 0; y < qrcode.size; y++) {
    for (uint8_t x = 0; x < qrcode.size; x++) {
      uint16_t color = qrcode_getModule(&qrcode, x, y) ? TFT_WHITE : TFT_BLACK;
      tft.fillRect(
        originX + x * pixelsPerModule,
        originY + y * pixelsPerModule,
        pixelsPerModule,
        pixelsPerModule,
        color
      );
    }
  }
}
