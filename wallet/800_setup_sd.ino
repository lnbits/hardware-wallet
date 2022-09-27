void setupSD() {
  SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS)) {
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