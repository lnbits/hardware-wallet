#define FORMAT_ON_FAIL true

void setup() {
  Serial.begin(9600);

  // load screen
  tft.init();
  tft.setRotation(1);
  tft.invertDisplay(true);
  showBootLogo();
  delay(1500);

  h.begin();
  FlashFS.begin(FORMAT_ON_FAIL);
  SPIFFS.begin(true);

  logInfo("HWW: waiting for commands");
  // In case of forced reboot, tell the client to logout.
  // Secure connection not established yet. Sendin in clear text.
  Serial.println(COMMAND_PASSWORD_CLEAR +  " 1");

  if (loadFiles() == false)
    showMessage("Failed to open files",  "Reset or 'help'");
  updateDeviceConfig();
  setupSD();
  waitForFirmwareHashConfirmation();

  // Give WebSerial the full pairing window after device setup is complete.
  global.startTime = millis();
}

bool loadFiles() {
  bool walletLoaded = loadWalletStorage();
  global.authenticated = false;
  global.mnemonic = "";
  global.passphrase = "";

  FileData sharedSecretFile = readFile(SPIFFS, global.sharedSecretFileName.c_str());
  if (sharedSecretFile.success) {
    deleteFile(SPIFFS, global.sharedSecretFileName.c_str());
  }

  return walletLoaded;
}

void updateDeviceConfig() {
  FileData deviceMetaFile = readFile(SPIFFS, global.deviceMetaFileName.c_str());

  if (deviceMetaFile.success) {
    global.deviceId = getWordAtPosition(deviceMetaFile.data, 0);

    String button1PinStr = getWordAtPosition(deviceMetaFile.data, 1);
    if (button1PinStr && button1PinStr != "") {
      global.button1Pin = button1PinStr.toInt();
    }

    String button2PinStr = getWordAtPosition(deviceMetaFile.data, 2);
    if (button2PinStr && button2PinStr != "") {
      global.button2Pin = button2PinStr.toInt();
    }
  } else {
    // create random unique ID
    const int uuidSize = 32;
    uint8_t uuid[uuidSize] = {0};
    String tempMnemonic = generateStrongerMnemonic(24);
    if (
      tempMnemonic == "" ||
      mnemonicToEntropy(tempMnemonic, uuid, uuidSize) != uuidSize
    ) {
      clearSensitiveBytes(uuid, sizeof(uuid));
      logInfo("Device ID creation aborted: RNG health check failed");
      return;
    }
    global.deviceId = toHex(uuid, uuidSize);
    clearSensitiveBytes(uuid, sizeof(uuid));
    writeFile(SPIFFS, global.deviceMetaFileName.c_str(), global.deviceId);
  }

  pinMode(global.button1Pin, INPUT_PULLUP);
  if (global.button1Pin != global.button2Pin) {
    pinMode(global.button2Pin, INPUT_PULLUP);
  }
}
