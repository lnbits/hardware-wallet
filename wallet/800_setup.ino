#define FORMAT_ON_FAIL true

void setup() {
  Serial.begin(9600);

  // load screen
  tft.init();
  tft.setRotation(1);
  tft.invertDisplay(true);
  logo();
  delay(3000);
  h.begin();
  FlashFS.begin(FORMAT_ON_FAIL);
  SPIFFS.begin(true);

  logSerial("HWW: waiting for commands");
  // In case of forced reboot, tell the client to logout.
  // Secure connection not established yet. Sendin in clear text.
  Serial.println(COMMAND_PASSWORD_CLEAR +  " 1");

  if (loadFiles() == false)
    showMessage("Failed to open files",  "Reset or 'help'");

}

bool loadFiles() {
  FileData pwdFile = readFile(SPIFFS, global.passwordFileName.c_str());
  String passwordHash = pwdFile.data;

  int byteSize =  passwordHash.length() / 2;
  byte passwordHashBin[byteSize];
  fromHex(passwordHash, passwordHashBin, byteSize);

  FileData mnFile = readFile(SPIFFS, global.mnemonicFileName.c_str());
  global.encrytptedMnemonic = decryptDataWithIv(passwordHashBin, mnFile.data);
  global.passwordHash = passwordHash;

  FileData sharedSecretFile = readFile(SPIFFS, global.sharedSecretFileName.c_str());
  logSerial("sharedSecretFile.success " + String(sharedSecretFile.success));
  if (sharedSecretFile.success) {
    logSerial("sharedSecretFile.data " + String(sharedSecretFile.data));
    fromHex(sharedSecretFile.data, global.dhe_shared_secret, sizeof(global.dhe_shared_secret));
  }

  FileData deviceMetaFile = readFile(SPIFFS, global.deviceMetaFileName.c_str());
  logSerial("deviceMetaFile.success " + String(deviceMetaFile.success));
  if (deviceMetaFile.success) {
    logSerial("sharedSecretFile.data " + deviceMetaFile.data);
    global.deviceUUID = deviceMetaFile.data;
  } else {
    // create random unique ID
    int uuidSize = 32;
    uint8_t uuid[uuidSize];
    String tempMnemonic = generateMnemonic(24);
    mnemonicToEntropy(tempMnemonic, uuid, uuidSize);
    global.deviceUUID = toHex(uuid, uuidSize);
    writeFile(SPIFFS, global.deviceMetaFileName.c_str(), global.deviceUUID);
  }

  return mnFile.success && pwdFile.success;
}
