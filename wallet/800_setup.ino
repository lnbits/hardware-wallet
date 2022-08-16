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
  // Secure connection not established yet. Sengin in clear text.
  Serial.println(COMMAND_PASSWORD_CLEAR +  " 1");

  if (loadFiles() == false)
    showMessage("Failed to open files",  "Reset or 'help'");

}

bool loadFiles() {
  FileData pwdFile = readFile(SPIFFS, "/hash.txt");
  String passwordHash = pwdFile.data;
  
  int byteSize =  passwordHash.length() / 2;
  byte passwordHashBin[byteSize];
  fromHex(passwordHash, passwordHashBin, byteSize);

  FileData mnFile = readFile(SPIFFS, "/mn.txt");
  global.encrytptedMnemonic = decryptDataWithIv(passwordHashBin, mnFile.data);
  global.passwordHash = passwordHash;

  return mnFile.success && pwdFile.success;
}
