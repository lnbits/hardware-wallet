#define FORMAT_ON_FAIL true

void setup() {
  Serial.begin(9600);

  // load screen
  tft.init();
  tft.setRotation(1);
  tft.invertDisplay(true);

  h.begin();
  FlashFS.begin(FORMAT_ON_FAIL);
  SPIFFS.begin(true);

  logInfo("HWW: waiting for commands");
  // In case of forced reboot, tell the client to logout.
  // Secure connection not established yet. Sendin in clear text.
  Serial.print(COMMAND_PASSWORD_CLEAR);
  Serial.println(" 1");

  if (loadFiles() == false)
    showMessage("Failed to open files",  "Reset or 'help'");
  updateDeviceConfig();
  setupSD();
}

bool loadFiles() {
  FileData pwdFile = readFile(SPIFFS, FILE_PASSWORD);
  String passwordHash = pwdFile.data;

  int byteSize =  passwordHash.length() / 2;
  byte passwordHashBin[byteSize];
  fromHex(passwordHash, passwordHashBin, byteSize);

  FileData mnFile = readFile(SPIFFS, FILE_MNEMONIC);
  global.mnemonic = decryptDataWithIv(passwordHashBin, mnFile.data);
  global.passwordHash = passwordHash;

  FileData sharedSecretFile = readFile(SPIFFS, FILE_SHAREDSECRET);
  if (sharedSecretFile.success) {
    fromHex(sharedSecretFile.data, global.dhe_shared_secret, sizeof(global.dhe_shared_secret));
  }

  return mnFile.success && pwdFile.success;
}

void updateDeviceConfig() {
  FileData deviceMetaFile = readFile(SPIFFS, FILE_DEVICEMETA);

  if (deviceMetaFile.success) {
    global.deviceId = getWordAtPosition(deviceMetaFile.data, 0);

    // String button1PinStr = getWordAtPosition(deviceMetaFile.data, 1);
    // if (button1PinStr && button1PinStr != "") {
    //   global.button1Pin = button1PinStr.toInt();
    // }

    // String button2PinStr = getWordAtPosition(deviceMetaFile.data, 2);
    // if (button2PinStr && button2PinStr != "") {
    //   global.button2Pin = button2PinStr.toInt();
    // }
  } else {
    // create random unique ID
    int uuidSize = 32;
    uint8_t uuid[uuidSize];
    String tempMnemonic = generateStrongerMnemonic(24);
    mnemonicToEntropy(tempMnemonic, uuid, uuidSize);
    global.deviceId = toHex(uuid, uuidSize);
    writeFile(SPIFFS, FILE_DEVICEMETA, global.deviceId);
  }

  pinMode(BTN_1, INPUT_PULLUP);
  if (BTN_1 != BTN_2) {
    pinMode(BTN_2, INPUT_PULLUP);
  }
}
