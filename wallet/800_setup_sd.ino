bool quickSdCardPresent(SPIClass &sdSpi, int8_t chipSelectPin) {
  pinMode(chipSelectPin, OUTPUT);
  digitalWrite(chipSelectPin, HIGH);
  sdSpi.beginTransaction(SPISettings(400000, MSBFIRST, SPI_MODE0));

  // SD cards require at least 74 clocks with CS high before CMD0. Use the
  // same 160-clock preamble and two reset attempts as the ESP32 SD library,
  // but stop after the short response window instead of entering its
  // multi-second card-initialization timeout when the slot is empty.
  for (uint8_t index = 0; index < 20; index++) sdSpi.transfer(0xFF);

  const uint8_t resetCommand[6] = {0x40, 0, 0, 0, 0, 0x95};
  bool present = false;
  for (uint8_t attempt = 0; attempt < 2 && !present; attempt++) {
    digitalWrite(chipSelectPin, LOW);
    for (uint8_t index = 0; index < sizeof(resetCommand); index++) {
      sdSpi.transfer(resetCommand[index]);
    }
    for (uint8_t index = 0; index < 9; index++) {
      uint8_t response = sdSpi.transfer(0xFF);
      // CMD0 must put an SD card into idle state (R1 = 0x01). In an empty
      // socket the floating MISO line can read as 0x00, which must not be
      // mistaken for a card or SD.begin() will enter its long timeout.
      if (response == 0x01) {
        present = true;
        break;
      }
    }
    digitalWrite(chipSelectPin, HIGH);
    sdSpi.transfer(0xFF);
    if (!present && attempt == 0) delay(20);
  }

  sdSpi.endTransaction();
  return present;
}

void setupSD() {
  if (!BOARD.hasSdCard) {
    global.hasCommandsFile = false;
    global.commands = "";
    return;
  }

  SPIClass &sdSpi = boardSdSpi();
  sdSpi.begin(
    BOARD.sdClockPin,
    BOARD.sdMisoPin,
    BOARD.sdMosiPin,
    BOARD.sdChipSelectPin
  );
  if (!quickSdCardPresent(sdSpi, BOARD.sdChipSelectPin)) {
    global.hasCommandsFile = false;
    global.commands = "";
    logInfo("No SD Card detected");
    return;
  }
  if (!SD.begin(BOARD.sdChipSelectPin, sdSpi)) {
    global.hasCommandsFile = false;
    global.commands = "";
    logInfo("No SD Card detected");
    return;
  }
  uint32_t cardSize = SD.cardSize() / (1024 * 1024);
  showMessage("SDCard Size: ",  String(cardSize) + "MB");
  delay(1000);

  FileData fd = readFile(SD, global.commandsInFileName.c_str());
  if (fd.success == true) {
    showMessage("SD Card", "Commands file found");
    global.hasCommandsFile = true;
    global.commands = fd.data;
    initCommandsFile();
  } else {
    showMessage("SD Card", "Commands file not found");
    global.hasCommandsFile = false;
    global.commands = "";
  }


  delay(2000);

}

void initCommandsFile() {
  writeFile(SD, global.commandsLogFileName.c_str(),  "#### commands file found ####\n");
  writeFile(SD, global.commandsOutFileName.c_str(),  "#### commands output ####\n");
}
