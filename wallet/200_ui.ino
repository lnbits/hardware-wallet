//========================================================================//
//===============================UI STUFF=================================//
//========================================================================//

const uint32_t THINKING_FRAME_INTERVAL_MS = 250;
bool thinkingAnimationActive = false;
uint8_t thinkingFrameIndex = 0;
uint32_t thinkingFrameChangedAt = 0;

void drawThinkingFrame(uint8_t frameIndex) {
  int16_t frameSize = min(
    int16_t(THINKING_FRAME_WIDTH),
    min(tft.width(), uiContentHeight())
  );
  int16_t x = (tft.width() - frameSize) / 2;
  int16_t y = (uiContentHeight() - frameSize) / 2;
  bool previousSwapBytes = tft.getSwapBytes();
  tft.setSwapBytes(true);
  if (frameSize == THINKING_FRAME_WIDTH) {
    tft.pushImage(
      x,
      y,
      THINKING_FRAME_WIDTH,
      THINKING_FRAME_HEIGHT,
      THINKING_FRAMES[frameIndex]
    );
  } else {
    uint16_t row[THINKING_FRAME_WIDTH];
    const uint16_t *frame = THINKING_FRAMES[frameIndex];
    for (int16_t targetY = 0; targetY < frameSize; targetY++) {
      int16_t sourceY = uint32_t(targetY) * THINKING_FRAME_HEIGHT / frameSize;
      for (int16_t targetX = 0; targetX < frameSize; targetX++) {
        int16_t sourceX = uint32_t(targetX) * THINKING_FRAME_WIDTH / frameSize;
        row[targetX] = pgm_read_word(
          &frame[sourceY * THINKING_FRAME_WIDTH + sourceX]
        );
      }
      tft.pushImage(x, y + targetY, frameSize, 1, row);
    }
  }
  tft.setSwapBytes(previousSwapBytes);
}

void startThinkingAnimation() {
  beginUiScreen();
  thinkingAnimationActive = true;
  thinkingFrameIndex = 0;
  thinkingFrameChangedAt = millis();
  drawThinkingFrame(thinkingFrameIndex);
}

void updateThinkingAnimation() {
  if (!thinkingAnimationActive) return;
  uint32_t now = millis();
  if (uint32_t(now - thinkingFrameChangedAt) < THINKING_FRAME_INTERVAL_MS) return;

  thinkingFrameIndex = (thinkingFrameIndex + 1) % THINKING_FRAME_COUNT;
  thinkingFrameChangedAt = now;
  drawThinkingFrame(thinkingFrameIndex);
}

void stopThinkingAnimation() {
  thinkingAnimationActive = false;
}

void logo(int counter) {
  static uint32_t pairingLayoutRevision = UINT32_MAX;
  bool pairing = counter > 0;
  int16_t contentHeight = tft.height();
  int16_t titleY = max(4, contentHeight / 5);
  String title = "Bowser HWW";
  String fittedTitle = pairing ? title + " 9" : title;
  uint8_t titleSize = uiFittedTextSize(fittedTitle, 3, tft.width() - 8);

  bool redrawLayout = !pairing || pairingLayoutRevision != uiScreenRevision();
  if (redrawLayout) {
    beginUiScreen();
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setTextSize(titleSize);
    tft.setCursor(4, titleY);
    tft.print(title);

    String subTitle = pairing ? "Open for pairing" : "ubitcoin powered signer";
    tft.setTextSize(uiFittedTextSize(
      subTitle,
      pairing ? 2 : 1,
      tft.width() - 8
    ));
    tft.setCursor(4, max(24, contentHeight * 3 / 5));
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.print(subTitle);

    String version = "firmware " + env.version;
    String fingerprint = "fingerprint ending ";
    if (global.firmwareFingerprintEnding == "") {
      fingerprint += "unavailable";
    } else {
      String ending = global.firmwareFingerprintEnding;
      ending.toUpperCase();
      fingerprint += ending;
    }
    uint8_t fingerprintSize = uiFittedTextSize(
      fingerprint,
      1,
      tft.width() - 8
    );
    uint8_t versionSize = uiFittedTextSize(version, 1, tft.width() - 8);
    int16_t fingerprintY = max(
      40,
      contentHeight - uiTextPixelHeight(fingerprintSize) - 2
    );
    int16_t versionY = max(
      40,
      fingerprintY - uiTextPixelHeight(versionSize) - 2
    );
    tft.setTextSize(versionSize);
    tft.setCursor(4, versionY);
    tft.print(version);
    tft.setTextSize(fingerprintSize);
    tft.setCursor(4, fingerprintY);
    tft.print(fingerprint);
    pairingLayoutRevision = pairing ? uiScreenRevision() : UINT32_MAX;
  }

  if (pairing) {
    int16_t counterX = 4 + uiTextPixelWidth(title + " ", titleSize);
    tft.fillRect(
      counterX,
      titleY,
      tft.width() - counterX,
      uiTextPixelHeight(titleSize),
      TFT_BLACK
    );
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setTextSize(titleSize);
    tft.setCursor(counterX, titleY);
    tft.print(counter);
  }
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
  static uint32_t seedWordLayoutRevision = UINT32_MAX;
  if (seedWordLayoutRevision != uiScreenRevision()) {
    beginUiScreen(UiControls::PreviousNext);
    seedWordLayoutRevision = uiScreenRevision();
  }

  int16_t contentHeight = uiContentHeight();
  int16_t wordY = contentHeight / 2;
  wordY = wordY < 54 ? wordY : 54;
  tft.fillRect(0, 0, tft.width(), uiTextPixelHeight(uiTextSize(2)) + 10, TFT_BLACK);
  tft.fillRect(
    0,
    wordY,
    tft.width(),
    min(uiTextPixelHeight(uiTextSize(3)) + 2, contentHeight - wordY),
    TFT_BLACK
  );

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  String heading = "Word " + position + ":";
  tft.setTextSize(uiFittedTextSize(heading, 2, tft.width() - 8));
  tft.setCursor(4, 8);
  tft.println(heading);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(uiFittedTextSize(word, 3, tft.width() - 8));
  tft.setCursor(4, wordY);
  tft.println(word);
}

void showDiceRollProgress(int rollCount, char latestRoll) {
  static uint32_t diceLayoutRevision = UINT32_MAX;
  bool complete = rollCount == DICE_ROLL_COUNT;
  bool startOfEntry = rollCount == 0 && latestRoll == 0;
  bool redrawLayout = startOfEntry || diceLayoutRevision != uiScreenRevision();
  if (redrawLayout) {
    beginUiScreen(UiControls::DicePad, complete);
  } else {
    setUiControls(UiControls::DicePad, complete);
  }

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  int16_t y = 2;
  uint8_t textSize = uiFittedTextSize("Dice wallet", 2, tft.width() - 8);
  if (redrawLayout) {
    tft.setTextSize(textSize);
    tft.setCursor(4, y);
    tft.println("Dice wallet");
  }
  y += uiTextPixelHeight(textSize) + 2;
  textSize = uiFittedTextSize("Enter rolls 1-6", 2, tft.width() - 8);
  if (redrawLayout) {
    tft.setTextSize(textSize);
    tft.setCursor(4, y);
    tft.println("Enter rolls 1-6");
  }
  y += uiTextPixelHeight(textSize) + 2;

  // Keep the static instructions and touch controls in place. Only the
  // progress/status area changes as rolls are added or removed.
  tft.fillRect(0, y, tft.width(), uiContentHeight() - y, TFT_BLACK);
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
  diceLayoutRevision = uiScreenRevision();
}

void showDiceMnemonicWord(int position, String word) {
  static uint32_t diceWordLayoutRevision = UINT32_MAX;
  if (diceWordLayoutRevision != uiScreenRevision()) {
    beginUiScreen(UiControls::PreviousNext);
    diceWordLayoutRevision = uiScreenRevision();
  }

  int16_t contentHeight = uiContentHeight();
  int16_t wordY = contentHeight / 2;
  wordY = wordY < 50 ? wordY : 50;
  tft.fillRect(0, 0, tft.width(), uiTextPixelHeight(uiTextSize(2)) + 6, TFT_BLACK);
  tft.fillRect(
    0,
    wordY,
    tft.width(),
    min(uiTextPixelHeight(uiTextSize(3)) + 2, contentHeight - wordY),
    TFT_BLACK
  );
  if (!BOARD.hasTouchscreen) {
    tft.fillRect(0, contentHeight - 12, tft.width(), 12, TFT_BLACK);
  }

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  String heading = "Write word " + String(position);
  tft.setTextSize(uiFittedTextSize(heading, 2, tft.width() - 8));
  tft.setCursor(4, 4);
  tft.println(heading);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(uiFittedTextSize(word, 3, tft.width() - 8));
  tft.setCursor(4, wordY);
  tft.println(word);
  if (!BOARD.hasTouchscreen) {
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    String guidance = position == 24 ? "* back    # finish" : "* back    # next";
    tft.setTextSize(uiFittedTextSize(guidance, 1, tft.width() - 8));
    tft.setCursor(4, contentHeight - 12);
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
